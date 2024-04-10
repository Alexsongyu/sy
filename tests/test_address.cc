#include "sy/address.h"
#include "sy/log.h"

sy::Logger::ptr g_logger = SY_LOG_ROOT();

// 通过域名、IP、主机名获得对应Address
void test() {
    std::vector<sy::Address::ptr> addrs;

    SY_LOG_INFO(g_logger) << "begin";
    bool v = sy::Address::Lookup(addrs, "localhost:3080");
    //bool v = sy::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //bool v = sy::Address::Lookup(addrs, "www.sy.top", AF_INET);
    SY_LOG_INFO(g_logger) << "end";
    if(!v) {
        SY_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        SY_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = sy::Address::LookupAny("localhost:4080");
    if(addr) {
        SY_LOG_INFO(g_logger) << *addr;
    } else {
        SY_LOG_ERROR(g_logger) << "error";
    }
}

// 通过静态方法获得本机的Address
void test_iface() {
    std::multimap<std::string, std::pair<sy::Address::ptr, uint32_t> > results;

    bool v = sy::Address::GetInterfaceAddresses(results);
    if(!v) {
        SY_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        SY_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = sy::IPAddress::Create("www.sy.top");
    auto addr = sy::IPAddress::Create("127.0.0.8");
    if(addr) {
        SY_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    //test_ipv4();
    //test_iface();
    test();
    return 0;
}
