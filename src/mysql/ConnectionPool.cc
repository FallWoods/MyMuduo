#include "ConnectionPool.h"
#include "json.hpp"
#include <fstream>
#include <thread>

using json = nlohmann::json;

bool ConnectionPool::parseJsonFile() {
    // std::ifstream ifs("dbconf.json");
    // auto conf = json::parse(ifs);

    ip_ = "192.168.10.135";
    user_ = "remote_user";
    password_ = "jiuyouziqu0317@Li";
    dbName_ = "testbase";
    port_ = 3306;
    minSize_ = 100;
    maxSize_ = 1024;
    timeout_ = 100;
    maxIdleTime_ = 5000;
    return true;
}

ConnectionPool::ConnectionPool() {
    // 加载配置文件
    if (!parseJsonFile()) {
        return;
    }
    for (int i = 0; i < minSize_; ++i) {
        MysqlConn* conn = new MysqlConn;
        conn->connect(user_, password_, dbName_, ip_, port_);
        connQueue_.push(conn);
    }
    std::thread producer(&ConnectionPool::produceConnection, this);
    std::thread recycler(&ConnectionPool::recycleConnection, this);
    producer.detach();
    recycler.detach();
}

ConnectionPool::~ConnectionPool() {
    while (!connQueue_.empty()) {
        MysqlConn* conn = connQueue_.front();
        connQueue_.pop();
        delete conn;
    }
}

std::shared_ptr<MysqlConn> ConnectionPool::getConnection(){
    std::unique_lock<std::mutex> lk(m_);
    while (connQueue_.empty()) {
        if (std::cv_status::timeout == cond_.wait_for(lk, std::chrono::milliseconds(timeout_))) {
            if (connQueue_.empty()) {
                continue;
            } else {
                break;
            }
        }
    }
    MysqlConn* conn = connQueue_.front();
    connQueue_.pop();
    // 取出一个连接后，唤醒生产者，因为可能有多个消费者，所以使用notify_all()，确保生产者被唤醒
    cond_.notify_all();
    // 自定义删除器，以使我们使用完数据库连接后，此连接能被自动回收回队列
    return std::shared_ptr<MysqlConn>(conn, [this](MysqlConn* conn){
        std::lock_guard<std::mutex> lk(this->m_);
        this->connQueue_.push(conn);
        conn->refreshAliveTime();
    });
}

void ConnectionPool::produceConnection() {
    while (true) {
        std::unique_lock<std::mutex> lk(m_);
        while (connQueue_.size() >= minSize_) {
            cond_.wait(lk);
        }
        addConnection();
        // 唤醒消费者，因为生产者只有一个，此时，阻塞的只有消费者，所以用notify_one()即可
        cond_.notify_one();
    }
}

void ConnectionPool::addConnection() {
    MysqlConn* conn = new MysqlConn;
    conn->connect(user_, password_, dbName_, ip_, port_);
    connQueue_.push(conn);
}

void ConnectionPool::recycleConnection() {
    while (true) {
        // 阻塞500毫秒
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::lock_guard<std::mutex> lk(m_);
        while (connQueue_.size() > minSize_) {
            MysqlConn* conn = connQueue_.front();
            if (conn->getAliveTime() >= maxIdleTime_) {
                // 如果超时，就删除，并继续访问下一个连接
                connQueue_.pop();
                delete conn;
            } else {
                // 如果队头的连接没超时，则说明所有的连接都没有超时
                break;
            }
        }
    }
}
