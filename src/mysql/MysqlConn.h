#pragma once

#include <iostream>
#include <mysql/mysql.h>
#include <chrono>
#include <string>
using std::chrono::steady_clock;

class MysqlConn {
public:
    // 初始化数据库连接
    MysqlConn();
    // 释放数据库连接
    ~MysqlConn();
    // 连接指定数据库
    bool connect(std::string user,
                std::string password,
                std::string dbName,
                std::string ipaddr,
                unsigned int port = 3306);
    // 更新数据库
    bool update(std::string sql);
    // 查询数据库
    bool query(std::string sql);
    // 遍历查询得到的结果集
    bool next();
    // 得到结果集中的字段值
    std::string value(int index);

    // 事物操作
    bool transaction() {
        return mysql_autocommit(conn_, false);
    }
    // 提交事务
    bool commit() {
        return mysql_commit(conn_);
    }
    // 事务回滚
    bool rollback() {
        return mysql_rollback(conn_);
    }

    // 刷新起始的空闲时刻
    void refreshAliveTime() {
        alivetime_ = std::chrono::steady_clock::now();
    }
    // 计算连接的存活时间
    long long getAliveTime() {
        std::chrono::nanoseconds res = std::chrono::steady_clock::now() - alivetime_;
        std::chrono::milliseconds milisecs = std::chrono::duration_cast<std::chrono::milliseconds>(res);
        return milisecs.count();
    }
private:
    // 释放结果集
    void freeResult();

    // 一个连接对象
    MYSQL* conn_ = nullptr;
    // 结果集
    MYSQL_RES* result_ = nullptr;
    // 结果集的一行，用来遍历结果集
    MYSQL_ROW row_ = nullptr;

    // 绝对时钟
    std::chrono::steady_clock::time_point alivetime_;
};