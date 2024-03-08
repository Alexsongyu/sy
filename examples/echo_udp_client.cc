#include "sy/socket.h"
#include "sy/iomanager.h"
#include "sy/log.h"
#include <stdlib.h>

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

const char* ip = nullptr;
uint16_t port = 0;

void run() {
    sy::IPAddress::ptr addr = sy::Address::LookupAnyIPAddress(ip);
    if(!addr) {
        SY_LOG_ERROR(g_logger) << "invalid ip: " << ip;
        return;
    }
    addr->setPort(port);

    sy::Socket::ptr sock = sy::Socket::CreateUDP(addr);

    sy::IOManager::GetThis()->schedule([sock](){
            sy::Address::ptr addr(new sy::IPv4Address);
            SY_LOG_INFO(g_logger) << "begin recv";
            while(true) {
                char buff[1024];
                int len = sock->recvFrom(buff, 1024, addr);
                if(len > 0) {
                    std::cout << std::endl << "recv: " << std::string(buff, len) << " from: " << *addr << std::endl;
                }
            }
    });
    sleep(1);
    while(true) {
        std::string line;
        std::cout << "input>";
        std::getline(std::cin, line);
        if(!line.empty()) {
            int len = sock->sendTo(line.c_str(), line.size(), addr);
            if(len < 0) {
                int err = sock->getError();
                SY_LOG_ERROR(g_logger) << "send error err=" << err
                        << " errstr=" << strerror(err) << " len=" << len
                        << " addr=" << *addr
                        << " sock=" << *sock;
            } else {
                SY_LOG_INFO(g_logger) << "send " << line << " len:" << len;
            }
        }
    }
}

int main(int argc, char** argv) {
    if(argc < 3) {
        SY_LOG_INFO(g_logger) << "use as[" << argv[0] << " ip port]";
        return 0;
    }
    ip = argv[1];
    port = atoi(argv[2]);
    sy::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
