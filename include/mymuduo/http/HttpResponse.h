#pragma once

#include <unordered_map>

class Buffer;

class HttpResponse {
public:
    // 响应状态码
    enum HttpStatusCode {
        UNKNOWN,
        K200OK = 200,
        K301MOVEDPERMANENTLY = 301,
        K400BADREQUEST = 400,
        K404NotFound =404,
    };

    explicit HttpResponse(bool close) : closeConnection_(close) {}
    
    void setStatusCode(HttpStatusCode code) { statusCode_ = code; }

    void setStatusMessage(const std::string& message) {
        statusMessage_ = message;
    }
    void setCloseConnection(bool on) {
        closeConnection_ =  on;
    }
    bool closeConnection() const { return closeConnection_; }

    void setContentType(const std::string& contentType) {
        addHeader("Content-Type", contentType);
    }

    void addHeader(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }

    void setBody(const std::string& body){ body_ = body; }

    void appendToBuffer(Buffer* output) const;
private:
    std::unordered_map<std::string, std::string> headers_;
    HttpStatusCode statusCode_ = UNKNOWN;
    // FIXME: add http version
    // 描述状态信息的原因短语
    std::string statusMessage_;
    bool closeConnection_;
    std::string body_;
};