#include "sy/socket.h"
#include "sy/log.h"
#include "sy/iomanager.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

void run() {
    sy::IPAddress::ptr addr = sy::Address::LookupAnyIPAddress("0.0.0.0:8050");
    sy::Socket::ptr sock = sy::Socket::CreateUDP(addr);
    if(sock->bind(addr)) {
        SY_LOG_INFO(g_logger) << "udp bind : " << *addr;
    } else {
        SY_LOG_ERROR(g_logger) << "udp bind : " << *addr << " fail";
        return;
    }
    while(true) {
        char buff[1024];
        sy::Address::ptr from(new sy::IPv4Address);
        int len = sock->recvFrom(buff, 1024, from);
        if(len > 0) {
            buff[len] = '\0';
            SY_LOG_INFO(g_logger) << "recv: " << buff << " from: " << *from;
            len = sock->sendTo(buff, len, from);
            if(len < 0) {
                SY_LOG_INFO(g_logger) << "send: " << buff << " to: " << *from
                    << " error=" << len;
            }
        }
    }
}

int main(int argc, char** argv) {
    sy::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
