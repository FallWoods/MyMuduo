#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <string>
#include <unordered_map>

class HttpRequest {
public:
    enum Method {
        INVALID,
        GET,
        POST,
        HEAD,
        PUT,
        DELETE
    };

    enum Version {
        UNKNOWN,
        HTTP10,
        HTTP11
    };

    HttpRequest() {}

    void setVersion(Version v) { version_ = v; }
    Version version() const { return version_; }

    bool setMethod(const char* start, const char* end) {
        std::string m(start, end);
        if (m == "GET") {
            method_ = GET;
        } else if (m == "POST") {
            method_ = POST;
        } else if (m == "HEAD") {
            method_ = HEAD;
        } else if (m == "PUT") {
            method_ = PUT;
        } else if (m == "DELETE") {
            method_ = DELETE;
        } else {
            method_ = INVALID;
        }
        // 判断method_是否合法
        return method_ != INVALID;
    }

    Method method() const { return method_; }

    const char* methodString() const {
        std::string res = "INVALID";
        switch (method_) {
            case GET :
                res = "GET";
                break;
            case POST :
                res = "POST";
                break;
            case HEAD :
                res = "HEAD";
                break;
            case PUT :
                res = "PUT";
                break;
            case DELETE :
                res = "DELETE";
                break;
            default :
                break;
        }
        return res.c_str();
    }

    void setPath(const char* start, const char* end) { path_.assign(start, end); }
    const std::string& path() const { return path_; }

    void setQuery(const char* start, const char* end) { query_.assign(start, end); }
    const std::string& query() const { return query_; }

    void setReceiveTime(Timestamp t) { receiveTime_ = t; }
    Timestamp receiveTime() const { return receiveTime_; }

    void addHeader(const char* start, const char* colon, const char* end) {
        std::string field(start, colon);
        ++colon;
        // 跳过vlaue前面的空格
        while (colon < end && isspace(*colon)) ++colon;
        std::string value(colon, end);
        // 丢掉value后面的空格，通过重新截断大小完成
        while (!value.empty() && isspace(value[value.size()-1])) value.resize(value.size()-1);
        headers_[field] = value;
    }

    // 获取请求首部的对应值，不存在时，返回空字符串
    std::string getHeadder(const std::string& field) const {
        std::string res;
        auto it = headers_.find(field);
        if (it != headers_.end()) {
            res = it->second;
        }
        return res;
    }

    const std::unordered_map<std::string, std::string>& headers() const {
        return headers_;
    }

    void swap(HttpRequest& other) {
        using std::swap;
        swap(other.method_, method_);
        swap(other.version_, version_);
        swap(other.path_, path_);
        swap(other.query_, query_);
        swap(other.receiveTime_, receiveTime_);
        swap(other.headers_, headers_);
    }
private:
    Method method_ = INVALID;             // 请求方法
    Version version_ = UNKNOWN;           // 协议版本号
    std::string path_;                    // 请求路径，即所请求的资源的URI
    std::string query_;                   // 询问参数，即URI后跟？后的参数
    Timestamp receiveTime_ ;              // 请求时间
    std::unordered_map<std::string, std::string> headers_;  //请求首部
};