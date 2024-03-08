#include "sy/http/http.h"
#include "sy/log.h"

void test_request() {
    sy::http::HttpRequest::ptr req(new sy::http::HttpRequest);
    req->setHeader("host" , "www.sy.top");
    req->setBody("hello sy");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    sy::http::HttpResponse::ptr rsp(new sy::http::HttpResponse);
    rsp->setHeader("X-X", "sy");
    rsp->setBody("hello sy");
    rsp->setStatus((sy::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
