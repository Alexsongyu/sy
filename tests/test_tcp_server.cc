#include "sy/tcp_server.h"
#include "sy/iomanager.h"
#include "sy/log.h"

sy::Logger::ptr g_logger = SY_LOG_ROOT();

void run() {
    auto addr = sy::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = sy::UnixAddress::ptr(new sy::UnixAddress("/tmp/unix_addr"));
    std::vector<sy::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    sy::TcpServer::ptr tcp_server(new sy::TcpServer);
    std::vector<sy::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    sy::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
