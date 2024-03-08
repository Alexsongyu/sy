#include "sy/socket.h"
#include "sy/sy.h"
#include "sy/iomanager.h"

static sy::Logger::ptr g_looger = SY_LOG_ROOT();

void test_socket() {
    //std::vector<sy::Address::ptr> addrs;
    //sy::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //sy::IPAddress::ptr addr;
    //for(auto& i : addrs) {
    //    SY_LOG_INFO(g_looger) << i->toString();
    //    addr = std::dynamic_pointer_cast<sy::IPAddress>(i);
    //    if(addr) {
    //        break;
    //    }
    //}
    sy::IPAddress::ptr addr = sy::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr) {
        SY_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        SY_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    sy::Socket::ptr sock = sy::Socket::CreateTCP(addr);
    addr->setPort(80);
    SY_LOG_INFO(g_looger) << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        SY_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        SY_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        SY_LOG_INFO(g_looger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        SY_LOG_INFO(g_looger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    SY_LOG_INFO(g_looger) << buffs;
}

void test2() {
    sy::IPAddress::ptr addr = sy::Address::LookupAnyIPAddress("www.baidu.com:80");
    if(addr) {
        SY_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        SY_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    sy::Socket::ptr sock = sy::Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        SY_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        SY_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = sy::GetCurrentUS();
    for(size_t i = 0; i < 10000000000ul; ++i) {
        if(int err = sock->getError()) {
            SY_LOG_INFO(g_looger) << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        //struct tcp_info tcp_info;
        //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    SY_LOG_INFO(g_looger) << "err";
        //    break;
        //}
        //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    SY_LOG_INFO(g_looger)
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = sy::GetCurrentUS();
            SY_LOG_INFO(g_looger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

int main(int argc, char** argv) {
    sy::IOManager iom;
    //iom.schedule(&test_socket);
    iom.schedule(&test2);
    return 0;
}
