#include "HttpRequest.h"

class Buffer;

class HttpContext {
public:
    // HTTP请求状态
    enum HttpRequestParseState {
        EXPECTREQUESTLINE,  // 解析请求行状态
        EXPECTHEADERS,      // 解析首部状态
        EXPECTBODY,         // 解析请求体状态
        GOTALL,             // 解析完毕状态
    };

    HttpContext(){}

    bool parseRequest(Buffer* buf, Timestamp receiveTime);

    bool gotAll() const { return state_ == GOTALL; }

    // 重置HttpContext状态，异常安全
    void reset() {
        state_ = EXPECTREQUESTLINE;
        /**
         * 构造一个临时空HttpRequest对象，和当前的成员HttpRequest对象交换置空
         * 然后临时对象析构
         */
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const { return request_; }

    HttpRequest& request() { return request_; }
private:
    // 解析请求行，即起始行
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState state_ = EXPECTREQUESTLINE;
    HttpRequest request_;
};