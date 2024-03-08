#include "sy/http/ws_server.h"
#include "sy/log.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

void run() {
    sy::http::WSServer::ptr server(new sy::http::WSServer);
    sy::Address::ptr addr = sy::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        SY_LOG_ERROR(g_logger) << "get address error";
        return;
    }
    auto fun = [](sy::http::HttpRequest::ptr header
                  ,sy::http::WSFrameMessage::ptr msg
                  ,sy::http::WSSession::ptr session) {
        session->sendMessage(msg);
        return 0;
    };

    server->getWSServletDispatch()->addServlet("/sy", fun);
    while(!server->bind(addr)) {
        SY_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    server->start();
}

int main(int argc, char** argv) {
    sy::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
