#pragma once

#include "MysqlConn.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

class ConnectionPool {
public:
    // 通过此函数获取唯一实例
    static ConnectionPool& getConnectionPool() {
        static ConnectionPool pool;
        return pool;
    }
    
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;

    ConnectionPool& operator=(const ConnectionPool&) = delete;

    std::shared_ptr<MysqlConn> getConnection();
private:
    // 单例模式
    ConnectionPool();
    bool parseJsonFile();
    // 新增数据库连接
    void produceConnection();
    // 当数据库连接太多时，回收数据库连接
    void recycleConnection();
    // 增加一个数据库连接
    void addConnection();
    std::queue<MysqlConn*> connQueue_;
    std::string ip_;
    unsigned short port_;
    std::string user_;
    std::string password_;
    std::string dbName_;

    // 连接池的最小大小
    int minSize_;
    // 连接池的最大大小
    int maxSize_;

    int timeout_;
    int maxIdleTime_;

    std::mutex m_;
    std::condition_variable cond_;
};