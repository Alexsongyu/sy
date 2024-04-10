#ifndef __SY_ADDRESS_H__
#define __SY_ADDRESS_H__

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>

namespace sy {

class IPAddress;

// 抽象类，对应sockaddr类型。提供获取地址的方法，以及一些基础操作的纯虚函数
class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    // 通过sockaddr指针创建Address
    // 输入addr sockaddr指针，addrlen sockaddr的长度
    // 返回和sockaddr相匹配的Address，失败返回nullptr
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

    // 通过host地址返回对应条件的所有Address
    // 输出：result 保存满足条件的Address
    // 输入：host 域名,服务器名。family 协议族。type socketl类型。protocol 协议
    // 返回是否转换成功
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    // 通过host地址返回对应条件的任意Address
    // 返回满足条件的任意Address,失败返回nullptr
    static Address::ptr LookupAny(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    // 通过host地址返回对应条件的任意IPAddress
    // 返回满足条件的任意IPAddress,失败返回nullptr
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);

    // 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
    // result 保存本机所有地址
    // 判断是否获取成功
    static bool GetInterfaceAddresses(std::multimap<std::string
                    ,std::pair<Address::ptr, uint32_t> >& result,
                    int family = AF_INET);
    // 获取指定网卡的地址和子网掩码位数
    // iface 网卡名称
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family = AF_INET);

    virtual ~Address() {}

    // 获取协议族
    int getFamily() const;

    // 获得sockaddr指针
    virtual const sockaddr* getAddr() const = 0;
    virtual sockaddr* getAddr() = 0;

    // 获取sockaddr的长度
    virtual socklen_t getAddrLen() const = 0;

    // 可读性输出地址
    virtual std::ostream& insert(std::ostream& os) const = 0;

    // 返回可读性字符串
    std::string toString() const;

    // 小于号比较函数
    bool operator<(const Address& rhs) const;

    // 等于函数
    bool operator==(const Address& rhs) const;

    // 不等于函数
    bool operator!=(const Address& rhs) const;
};

// IP地址，提供关于IP操作的纯虚函数
class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    // 通过域名、IP、服务器名创建IPAddress
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    // 获取该地址的广播地址：prefix_len 子网掩码位数
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

    // 获取该地址的网段
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;

    // 获取子网掩码地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    // 获取端口号
    virtual uint32_t getPort() const = 0;

    // 设置端口号
    virtual void setPort(uint16_t v) = 0;
};

// IPv4地址，对应sockaddr_in类型
class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    // 使用点分十进制地址创建IPv4Address
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    // 通过sockaddr_in构造IPv4Address
    IPv4Address(const sockaddr_in& address);

    // 通过二进制地址构造IPv4Address
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in m_addr;
};

// IPv6地址，对应sockaddr_in6类型
class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    IPv6Address();

    IPv6Address(const sockaddr_in6& address);

    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in6 m_addr;
};

// UnixSocket地址，对应sockaddr_un类型
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    UnixAddress();

    // 通过路径构造UnixAddress: path UnixSocket路径(长度小于UNIX_PATH_MAX)
    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::string getPath() const;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

// 未知地址，对应sockaddr类型
class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};

// 流式输出Address
std::ostream& operator<<(std::ostream& os, const Address& addr);

}

#endif
