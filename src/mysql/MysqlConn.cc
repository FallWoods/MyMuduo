#include "MysqlConn.h"

MysqlConn::MysqlConn() : conn_(mysql_init(nullptr)) {
    // 设置数据库字符串的编码，使支持中文
    mysql_set_character_set(conn_, "utf8");
}

MysqlConn::~MysqlConn() {
    freeResult();
    if (conn_) {
        mysql_close(conn_);
        conn_ = nullptr;
    }
}

bool MysqlConn::connect(std::string user,
                std::string password,
                std::string dbName,
                std::string ipaddr,
                unsigned int port) {
    // std::cout << "conn_：" << conn_ << std::endl;
    MYSQL* ptr = mysql_real_connect(conn_,
                        ipaddr.c_str(),
                        user.c_str(),
                        password.c_str(),
                        dbName.c_str(),
                        port, nullptr, 0);
    // std::cout << ptr << std::endl;
    // std::cout << mysql_error(conn_) << std::endl;
    refreshAliveTime();
    return ptr != nullptr ? true : false;
}

bool MysqlConn::update(std::string sql) {
    return mysql_query(conn_, sql.c_str()) ? false : true;
}

bool MysqlConn::query(std::string sql) {
    // 释放上次的结果
    freeResult();
    if (mysql_query(conn_, sql.c_str())) {
        return false;
    }
    result_ = mysql_store_result(conn_);
    return true;
}

bool MysqlConn::next() {
    if (!result_) return false;
    row_ = mysql_fetch_row(result_);
    if (row_) {
        return true;
    } else {
        return false;
    }
}

std::string MysqlConn::value(int index) {
    int fieldCount = mysql_num_fields(result_);
    if (index >= fieldCount || fieldCount <= 0) {
        return std::string();
    }
    char* val = row_[index];
    unsigned long val_length = mysql_fetch_lengths(result_)[index];
    return std::string(val,val_length);
}

void MysqlConn::freeResult() {
    if (!result_) return;
    mysql_free_result(result_);
    result_ = nullptr;
}