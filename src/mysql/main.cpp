#include "MysqlConn.h"
#include "ConnectionPool.h"
#include <iostream>
#include <sstream>
#include <thread>

void test1(int begin, int end) {
    // std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (int i = begin;i < end; ++i) {
        MysqlConn conn;
        conn.connect("remote_user", "jiuyouziqu0317@Li", "testbase","192.168.10.135");
        std::ostringstream os;
        os << "insert into person values(" << i << ", " << "45, 'man', 'person_" << i << "')";
        bool flag = conn.update(os.str().c_str());
    }
    // std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    // std::cout << "不使用连接池耗费的时间：" << (finish - start).count() << std::endl;
}

void test2(ConnectionPool* pool, int begin, int end) {
    // std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (int i = begin; i < end; ++i) {
        auto conn = pool->getConnection();
        std::ostringstream os;
        os << "insert into person values(" << i << ", " << "25, 'woman', 'person_" << i << "')";
        bool flag = conn->update(os.str().c_str());
    }
    // std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    // std::cout << "使用连接池耗费的时间：" << (finish - start).count() << std::endl;
}

void test3() {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::thread t1(test1,0,1000);
    std::thread t2(test1,1000,2000);
    std::thread t3(test1,2000,3000);
    std::thread t4(test1,3000,4000);
    std::thread t5(test1,4000,5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    std::cout << "多线程不使用连接池耗费的时间：" << (finish - start).count() << std::endl;
}

void test4(ConnectionPool* pool) {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::thread t1(test2, pool, 0,1000);
    std::thread t2(test2, pool, 1000,2000);
    std::thread t3(test2, pool, 2000,3000);
    std::thread t4(test2, pool, 3000,4000);
    std::thread t5(test2, pool, 4000,5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    std::cout << "多线程使用连接池耗费的时间：" << (finish - start).count() << std::endl;
}

int query() {
    MysqlConn conn;
    conn.connect("remote_user", "jiuyouziqu0317@Li", "testbase","192.168.10.135");
    std::string sql = "insert into person values(11, 45, 'man', 'jame')";
    bool flag = conn.update(sql);
    std::cout<< "flag value:" << flag << std::endl;
    sql = "select * from person";
    conn.query(sql);
    while (conn.next()) {
        std::cout << conn.value(0) << ","
                  << conn.value(1) << ","
                  << conn.value(2) << ","
                  << conn.value(3) <<std::endl;
    }
    return 0;
}

int main() {
    // query();
    // test1(12,5012);
    ConnectionPool& pool = ConnectionPool::getConnectionPool();
    // test2(&pool,5013,10013);
    test4(&pool);
    return 0;
}