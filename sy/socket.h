#ifndef __SY_SOCKET_H__
#define __SY_SOCKET_H__

#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "address.h"
#include "noncopyable.h"

namespace sy {

class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    // Socket类型
    enum Type {
        // TCP类型
        TCP = SOCK_STREAM,
        // UDP类型
        UDP = SOCK_DGRAM
    };

    // Socket协议簇
    enum Family {
        /// IPv4 socket
        IPv4 = AF_INET,
        /// IPv6 socket
        IPv6 = AF_INET6,
        /// Unix socket
        UNIX = AF_UNIX,
    };

    // 创建TCP Socket(满足地址类型)
    static Socket::ptr CreateTCP(sy::Address::ptr address);

    // 创建UDP Socket(满足地址类型)
    static Socket::ptr CreateUDP(sy::Address::ptr address);

    // 创建IPv4的TCP Socket
    static Socket::ptr CreateTCPSocket();

    // 创建IPv4的UDP Socket
    static Socket::ptr CreateUDPSocket();

    // 创建IPv6的TCP Socket
    static Socket::ptr CreateTCPSocket6();

    // 创建IPv6的UDP Socket
    static Socket::ptr CreateUDPSocket6();

    // 创建Unix的TCP Socket
    static Socket::ptr CreateUnixTCPSocket();

    // 创建Unix的UDP Socket
    static Socket::ptr CreateUnixUDPSocket();

    Socket(int family, int type, int protocol = 0);

    virtual ~Socket();

    // 获取发送超时时间(毫秒)
    int64_t getSendTimeout();

    void setSendTimeout(int64_t v);

    // 获取接受超时时间(毫秒)
    int64_t getRecvTimeout();

    void setRecvTimeout(int64_t v);

    // 获取sockopt 信息
    bool getOption(int level, int option, void* result, socklen_t* len);

    // 获取sockopt模板 
    template<class T>
    bool getOption(int level, int option, T& result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    bool setOption(int level, int option, const void* result, socklen_t len);

    template<class T>
    bool setOption(int level, int option, const T& value) {
        return setOption(level, option, &value, sizeof(T));
    }

    // 接收connect链接
    // 成功返回新连接的socket,失败返回nullptr
    virtual Socket::ptr accept();

    // 绑定地址
    virtual bool bind(const Address::ptr addr);

    // 连接地址
    // 输入addr 目标地址 和 timeout_ms 超时时间(毫秒)
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    virtual bool reconnect(uint64_t timeout_ms = -1);

    // 监听socket
    // 输入：backlog 未完成连接队列的最大长度
    // result 返回监听是否成功
    // 前提： 必须先 bind 成功
    virtual bool listen(int backlog = SOMAXCONN);

    // 关闭socket
    virtual bool close();

    // 发送数据：单数据块
    // 输入：buffer 待发送数据的内存，length 待发送数据的长度，flags 标志字
    // 返回： >0 发送成功对应大小的数据， =0 socket被关闭， <0 socket出错
    virtual int send(const void* buffer, size_t length, int flags = 0);

    // 发送数据：多数据块
    virtual int send(const iovec* buffers, size_t length, int flags = 0);

    // 指定地址发送数据：单数据块
    // to 发送的目标地址
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);

    // 指定地址发送数据：多数据块
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

    // 接收数据：单数据块
    virtual int recv(void* buffer, size_t length, int flags = 0);

    // 接收数据：多数据块
    virtual int recv(iovec* buffers, size_t length, int flags = 0);

    // 指定地址接收数据：单数据块
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);

    // 指定地址接收数据：多数据块
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    Address::ptr getRemoteAddress();

    Address::ptr getLocalAddress();

    int getFamily() const { return m_family;}

    int getType() const { return m_type;}

    int getProtocol() const { return m_protocol;}

    bool isConnected() const { return m_isConnected;}

    bool isValid() const;

    int getError();

    // 输出信息到流中
    virtual std::ostream& dump(std::ostream& os) const;

    virtual std::string toString() const;

    // 返回socket句柄
    int getSocket() const { return m_sock;}

    bool cancelRead();

    bool cancelWrite();

    bool cancelAccept();

    // 取消所有事件
    bool cancelAll();
protected:
    // 初始化socket
    void initSock();

    // 创建socket
    void newSock();

    // 初始化accept后用于通信的socket
    virtual bool init(int sock);
protected:
    // socket句柄
    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;
    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};

class SSLSocket : public Socket {
public:
    typedef std::shared_ptr<SSLSocket> ptr;

    static SSLSocket::ptr CreateTCP(sy::Address::ptr address);
    static SSLSocket::ptr CreateTCPSocket();
    static SSLSocket::ptr CreateTCPSocket6();

    SSLSocket(int family, int type, int protocol = 0);
    virtual Socket::ptr accept() override;
    virtual bool bind(const Address::ptr addr) override;
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;
    virtual bool listen(int backlog = SOMAXCONN) override;
    virtual bool close() override;
    virtual int send(const void* buffer, size_t length, int flags = 0) override;
    virtual int send(const iovec* buffers, size_t length, int flags = 0) override;
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0) override;
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0) override;
    virtual int recv(void* buffer, size_t length, int flags = 0) override;
    virtual int recv(iovec* buffers, size_t length, int flags = 0) override;
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0) override;
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0) override;

    bool loadCertificates(const std::string& cert_file, const std::string& key_file);
    virtual std::ostream& dump(std::ostream& os) const override;
protected:
    virtual bool init(int sock) override;
private:
    std::shared_ptr<SSL_CTX> m_ctx;
    std::shared_ptr<SSL> m_ssl;
};

// 流式输出socket
// 输入： os 输出流，sock Socket类
std::ostream& operator<<(std::ostream& os, const Socket& sock);

}

#endif
