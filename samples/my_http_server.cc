#include "sy/http/http_server.h"
#include "sy/log.h"

sy::Logger::ptr g_logger = SY_LOG_ROOT();
sy::IOManager::ptr worker;
void run() {
    g_logger->setLevel(sy::LogLevel::INFO);
    sy::Address::ptr addr = sy::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        SY_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    sy::http::HttpServer::ptr http_server(new sy::http::HttpServer(true, worker.get()));
    //sy::http::HttpServer::ptr http_server(new sy::http::HttpServer(true));
    bool ssl = false;
    while(!http_server->bind(addr, ssl)) {
        SY_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    if(ssl) {
        //http_server->loadCertificates("/home/apps/soft/sy/keys/server.crt", "/home/apps/soft/sy/keys/server.key");
    }

    http_server->start();
}

int main(int argc, char** argv) {
    sy::IOManager iom(1);
    worker.reset(new sy::IOManager(4, false));
    iom.schedule(run);
    return 0;
}
