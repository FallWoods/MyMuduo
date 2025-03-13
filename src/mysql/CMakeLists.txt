cmake_minimum_required(VERSION 3.27)

project(test)

add_executable(test main.cpp MysqlConn.h MysqlConn.cc json.hpp ConnectionPool.h ConnectionPool.cc)
target_link_libraries(test pthread libmysqlclient.so)