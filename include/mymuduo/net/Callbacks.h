// This is a public header file, it must only include public header files.

#pragma once

#include <functional>
#include <memory>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr){
    return ptr.get();
}

template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr){
    return ptr.get();
}

// Adapted from google-protobuf stubs/common.h
// see License in muduo/base/Types.h
//???
// template<typename To,typename From>
// inline std::shared_ptr<To> down_pointer_cast(const std::shared_ptr<From>& f){
//     if(false){
//         implicit_cast<From*,To*>(0);
//     }
//     return std::static_pointer_cast
// }

// All client visible callbacks go here.
class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using TimerCallback = std::function<void ()>;
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void (const TcpConnectionPtr&, size_t)>;

using MessageCallback = std::function<void(const TcpConnectionPtr&,Buffer*, Timestamp)>;
// the data has been read to (buf, len)

void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);