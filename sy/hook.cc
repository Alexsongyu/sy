#include "hook.h"
#include <dlfcn.h>

#include "config.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "macro.h"

sy::Logger::ptr g_logger = SY_LOG_NAME("system");
namespace sy {

static sy::ConfigVar<int>::ptr g_tcp_connect_timeout =
    sy::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

// 获取接口原始地址
// 使用宏来封装对每个原始接口地址的获取
#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

// 将hook_init()封装到一个结构体的构造函数中，并创建静态对象
// 能够在main函数运行之前就能将地址保存到函数指针变量当中
void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
// dlsym - 从一个动态链接库或者可执行文件中获取到符号地址。成功返回跟name关联的地址
// RTLD_NEXT 返回第一个匹配到的 "name" 的函数地址
// 取出原函数，赋值给新函数
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                SY_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

// 定时器超时条件
struct timer_info {
    int cancelled = 0;
};

// fd文件描述符，fun原始函数，hook_fun_name	hook的函数名称，timeout_so超时时间类型，args可变参数
template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&&... args) {
    if(!sy::t_hook_enable) { // 如果不hook，直接返回原接口
        return fun(fd, std::forward<Args>(args)...); // 将传入的可变参数args以原始类型的方式传递给函数fun，从而避免不必要的类型转换和拷贝，提高代码的效率和性能。
    }
    
    sy::FdCtx::ptr ctx = sy::FdMgr::GetInstance()->get(fd);  // 获取fd对应的FdCtx
    if(!ctx) { // 没有文件
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->isClose()) { // 句柄是否关闭
        errno = EBADF; // 坏文件描述符
        return -1;
    }

    if(!ctx->isSocket() || ctx->getUserNonblock()) { // 不是socket 或 用户设置了非阻塞
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so); // 获得超时时间
    std::shared_ptr<timer_info> tinfo(new timer_info); // 设置超时条件

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...); // 先执行fun 读数据或写数据 若函数返回值有效就直接返回
    while(n == -1 && errno == EINTR) { // 若中断则重试
        n = fun(fd, std::forward<Args>(args)...);
    }
    if(n == -1 && errno == EAGAIN) { // 若为阻塞状态
        errno = 0; // 重置EAGIN(errno = 11)，此处已处理，不再向上返回该错误
        sy::IOManager* iom = sy::IOManager::GetThis(); // 获得当前IO调度器
        sy::Timer::ptr timer; // 定时器
        std::weak_ptr<timer_info> winfo(tinfo); // tinfo的弱指针，可以判断tinfo是否已经销毁

        if(to != (uint64_t)-1) { // 设置了超时时间
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() { // 添加条件定时器
                auto t = winfo.lock();
                if(!t || t->cancelled) { // 定时器失效了
                    return;
                }
                t->cancelled = ETIMEDOUT; // 没错误的话设置为超时而失败
                iom->cancelEvent(fd, (sy::IOManager::Event)(event)); // 取消事件强制唤醒
            }, winfo);
        }

        int rt = iom->addEvent(fd, (sy::IOManager::Event)(event));
        // addEvent失败， 取消上面加的定时器
        if(SY_UNLIKELY(rt)) {
            SY_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } 
        // addEvent成功，把执行时间让出来
        // 只有两种情况会从这回来：
        // 1) 超时了， timer cancelEvent triggerEvent会唤醒回来
        // 2) addEvent数据回来了会唤醒回来
        else {
            sy::Fiber::GetThis()->yield();
            if(timer) {
                timer->cancel();
            }
            if(tinfo->cancelled) { // 从定时任务唤醒，超时失败
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;  // 数据来了就直接重新去操作
        }
    }
    
    return n;
}


extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;// 声明变量
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!sy::t_hook_enable) {
        return sleep_f(seconds);
    }

    sy::Fiber::ptr fiber = sy::Fiber::GetThis();
    sy::IOManager* iom = sy::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(sy::Scheduler::*)
            (sy::Fiber::ptr, int thread))&sy::IOManager::schedule
            ,iom, fiber, -1));
    sy::Fiber::GetThis()->yield();
    return 0;
}

int usleep(useconds_t usec) {
    if(!sy::t_hook_enable) {
        return usleep_f(usec);
    }
    sy::Fiber::ptr fiber = sy::Fiber::GetThis();
    sy::IOManager* iom = sy::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(sy::Scheduler::*)
            (sy::Fiber::ptr, int thread))&sy::IOManager::schedule
            ,iom, fiber, -1));
    sy::Fiber::GetThis()->yield();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!sy::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    sy::Fiber::ptr fiber = sy::Fiber::GetThis();
    sy::IOManager* iom = sy::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(sy::Scheduler::*)
            (sy::Fiber::ptr, int thread))&sy::IOManager::schedule
            ,iom, fiber, -1));
    sy::Fiber::GetThis()->yield();
    return 0;
}

// 创建socket
int socket(int domain, int type, int protocol) {
    if(!sy::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    sy::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!sy::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    sy::FdCtx::ptr ctx = sy::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    if(ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    // ----- 异步开始 -----
    // 先尝试连接
    int n = connect_f(fd, addr, addrlen);
    // 连接成功
    if (n == 0) {
        return 0;
    // 其他错误，EINPROGRESS表示连接操作正在进行中
    } else if (n != -1 || errno != EINPROGRESS) {
        return n;
    }
	
    sy::IOManager* iom = sy::IOManager::GetThis();
    sy::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    // 设置了超时时间
    if (timeout_ms != (uint64_t)-1) {
        // 加条件定时器
        timer = iom->addConditionTimer(timeout_ms, [iom, fd, winfo]() {
            auto t = winfo.lock();
            if (!t || t->cancelled) {
                return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, sy::IOManager::WRITE);
            }, winfo);
    }
    
    // 添加一个写事件
    int rt = iom->addEvent(fd, sy::IOManager::WRITE);
    if (rt == 0) {
        /* 	只有两种情况唤醒：
         * 	1. 超时，从定时器唤醒
         *	2. 连接成功，从epoll_wait拿到事件 */
        sy::Fiber::GetThis()->yield();
        if (timer) {
            timer->cancel();
        }
        // 从定时器唤醒，超时失败
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
      // 添加事件失败
    } else {
        if (timer) {
            timer->cancel();
        }
        SY_LOG_ERROR(sy::g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }
	
    int error = 0;
    socklen_t len = sizeof(int);
    // 获取套接字的错误状态
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    // 没有错误，连接成功
    if (!error) {
        return 0;
    // 有错误，连接失败
    } else {
        errno = error;
        return -1;
    }
}

// socket连接
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, sy::s_connect_timeout);
}

// 接收请求
int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", sy::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        sy::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

// 后续一系列发送和接收
ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", sy::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", sy::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", sy::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", sy::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", sy::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", sy::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", sy::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", sy::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", sy::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", sy::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

// 关闭socket
int close(int fd) {
    if(!sy::t_hook_enable) {
        return close_f(fd);
    }

    sy::FdCtx::ptr ctx = sy::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = sy::IOManager::GetThis();
        if(iom) { // 取消所有事件
            iom->cancelAll(fd);
        }
        sy::FdMgr::GetInstance()->del(fd); // 在文件管理中删除
    }
    return close_f(fd);
}

// 修改文件状态：对用户反馈是否是用户设置了非阻塞模式
int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                sy::FdCtx::ptr ctx = sy::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                sy::FdCtx::ptr ctx = sy::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg); 
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

// 对设备进行控制操作：打开或者关闭文件描述符的非阻塞模式
int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        sy::FdCtx::ptr ctx = sy::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!sy::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            sy::FdCtx::ptr ctx = sy::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
