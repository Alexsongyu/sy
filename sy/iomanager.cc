#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace sy {

static sy::Logger::ptr g_logger = SY_LOG_NAME("system");

enum EpollCtlOp {
};

static std::ostream& operator<< (std::ostream& os, const EpollCtlOp& op) {
    switch((int)op) {
#define XX(ctl) \
        case ctl: \
            return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
    }
#undef XX
}

static std::ostream& operator<< (std::ostream& os, EPOLL_EVENTS events) {
    if(!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SY_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    SY_ASSERT(events & event);
    // 触发该事件就将该事件从注册事件中删掉
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        // 使用地址传入就会将cb的引用计数-1
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        // 使用地址传入就会将fiber的引用计数-1
        ctx.scheduler->schedule(&ctx.fiber);
    }
    // 执行完毕将协程调度器置空
    ctx.scheduler = nullptr;
    return;
}

// 改造协程调度器，使其支持epoll，并重载tickle和idle，实现通知调度协程和IO协程调度功能

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    :Scheduler(threads, use_caller, name) {
    // 创建epoll实例
    m_epfd = epoll_create(5000);
    SY_ASSERT(m_epfd > 0);  // 成功时，这些系统调用将返回非负文件描述符。如果出错，则返回-1，并且将errno设置为指示错误。

    // 创建pipe，，用于进程间通信:获取m_tickleFds[2]，其中m_tickleFds[0]是管道的读端，m_tickleFds[1]是管道的写端
    int rt = pipe(m_tickleFds);
    SY_ASSERT(!rt); // 成功返回0，失败返回-1，并且设置errno。

    // 注册pipe读句柄的可读事件，用于tickle调度协程，通过epoll_event.data.fd保存描述符
    epoll_event event;
    memset(&event, 0, sizeof(epoll_event)); // 用0初始化event
    event.events = EPOLLIN | EPOLLET; // 注册读事件，设置边缘触发模式
    event.data.fd = m_tickleFds[0]; // fd关联pipe的读端

    // 非阻塞方式，配合边缘触发
    // 对一个打开的文件描述符执行一系列控制操作
    // F_SETFL: 获取/设置文件状态标志
    // O_NONBLOCK: 使I/O变成非阻塞模式，在读取不到数据或是写入缓冲区已满会马上return，而不会阻塞等待。
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SY_ASSERT(!rt);

    // 将管道的读描述符加入epoll多路复用，如果管道可读，idle中的epoll_wait会返回
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SY_ASSERT(!rt);

    contextResize(32);// 初始化socket事件上下文vector

    // 这里直接启动了schedule，也就是说IOManager创建即可调度协程
    start();
}

// 析构：等Scheduler调度完所有的任务，然后再关闭epoll句柄和pipe句柄，然后释放所有的FdContext
IOManager::~IOManager() {
    // 停止调度器
    stop();
    // 释放epoll
    close(m_epfd);
    // 释放pipe
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    // 释放 m_fdContexts 内存
    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
         // 没有才new新的
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

//获得当前IO调度器
IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

// 通知调度器有任务要调度
void IOManager::tickle() {
    // 如果当前没有空闲调度线程，那就没必要发通知
    if(!hasIdleThreads()) {
        return;
    }
    // 有任务来了，就往 pipe 里发送1个字节的数据，这样 epoll_wait 就会唤醒。待idle协程yield之后Scheduler::run就可以调度其他任务
    int rt = write(m_tickleFds[1], "T", 1);
    SY_ASSERT(rt == 1);
}

// 对于IOManager而言，必须等所有待调度的IO事件都执行完了才可以退出
bool IOManager::stopping() {
    return m_pendingEventCount == 0 && Scheduler::stopping();
}

// idle协程：调度器无任务时执行
// 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件就绪
// 调度器无调度任务时会阻塞idle协程上，对IO调度器而言，idle状态应该关注两件事
// 一是有没有新的调度任务，对应Schduler::schedule()，如果有新的调度任务，那应该立即退出idle状态，并执行对应的任务；
// 二是关注当前注册的所有IO事件有没有触发，如果有触发，那么应该执行IO事件对应的回调函数
void IOManager::idle() {
    SY_LOG_DEBUG(g_logger) << "idle";

    // 一次epoll_wait最多检测256个就绪事件，如果就绪事件超过了这个数，那么会在下轮epoll_wait继续处理
    const uint64_t MAX_EVNETS = 256;
    epoll_event* events = new epoll_event[MAX_EVNETS]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        // 获取下一个定时器的超时时间，顺便判断调度器是否停止
        uint64_t next_timeout = 0;
        if( SYLAR_UNLIKELY(stopping(next_timeout))) {
            SYLAR_LOG_DEBUG(g_logger) << "name=" << getName() << "idle stopping exit";
            break;
        }

        // 阻塞在epoll_wait上，等待事件发生或定时器超时
        int rt = 0;
        do{
            // 默认超时时间5秒，如果下一个定时器的超时时间大于5秒，仍以5秒来计算超时，避免定时器超时时间太大时，epoll_wait一直阻塞
            static const int MAX_TIMEOUT = 5000;
            if(next_timeout != ~0ull) {
                next_timeout = std::min((int)next_timeout, MAX_TIMEOUT);
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
            if(rt < 0 && errno == EINTR) {
                continue;
            } else {
                break;
            }
        } while(true);

        // 收集所有已超时的定时器，执行回调函数
        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) {
            for(const auto &cb : cbs) {
                schedule(cb);
            }
            cbs.clear();
        }
        
        // 遍历所有发生的事件，根据epoll_event的私有指针找到对应的FdContext，进行事件处理
        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];// 从 events 中拿一个 event
            if(event.data.fd == m_tickleFds[0]) { // 如果获得的这个信息时来自 pipe
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);// 将 pipe 发来的1个字节数据读掉
                continue;
            }

            // 通过epoll_event的私有指针获取FdContext
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            // EPOLLERR: 出错，比如写读端已经关闭的pipe
            // EPOLLHUP: 套接字对端关闭
            // 出现这两种事件，应该同时触发fd的读和写事件，否则有可能出现注册的事件永远执行不到的情况
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            int real_events = NONE;
            // 读事件
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            // 写事件
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }
            // 没事件
            if((fd_ctx->events & real_events) == NONE) {
                continue;
            }
            // 剔除已经发生的事件，将剩下的事件重新加入epoll_wait，
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL; // 如果执行完该事件还有事件则修改，若无事件则删除
            event.events = EPOLLET | left_events;// 更新新的事件

            // 重新注册事件
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                SY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            // 处理已经发生的事件，也就是让调度器调度指定的函数或协程  
            if(real_events & READ) { // 读事件好了，执行读事件 
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) { // 写事件好了，执行写事件
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        // 执行完epoll_wait返回的事件
        // 一旦处理完所有的事件，idle协程yield，这样可以让调度协程(Scheduler::run)重新检查是否有新任务要调度
        // 上面triggerEvent实际也只是把对应的fiber重新加入调度，要执行的话还要等idle协程退出
        Fiber::ptr cur = Fiber::GetThis(); // 获得当前协程
        auto raw_ptr = cur.get(); // 获得裸指针
        cur.reset(); // 将当前idle协程指向空指针，状态为INIT

        raw_ptr->swapOut(); // 执行完返回scheduler的MainFiber 继续下一轮
    }
}

// 添加事件回调addEvent，删除事件回调delEvent，取消事件回调cancelEvent，取消全部事件cancelAll

// 添加事件：fd描述符发生了event事件时执行cb函数
// 参数：fd socket句柄，event 事件类型，cb 事件回调函数，如果为空，则默认把当前协程作为回调执行体
// 添加成功返回0,失败返回-1 
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    // 初始化一个 FdContext：找到fd对应的FdContext，如果不存在，那就分配一个
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    // 从 m_fdContexts 中拿到对应的 FdContext
    if((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5); // 不够就扩充
        fd_ctx = m_fdContexts[fd];
    }

    // 同一个fd不允许重复添加相同的事件
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(SY_UNLIKELY(fd_ctx->events & event)) {
        SY_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << (EPOLL_EVENTS)event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
        SY_ASSERT(!(fd_ctx->events & event));
    }

    // 将新的事件加入epoll_wait，使用epoll_event的私有指针存储FdContext的位置
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD; // 若已经有注册的事件则为修改操作，若没有则为添加操作
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;// 设置边缘触发模式，添加原有的事件以及要注册的事件
    epevent.data.ptr = fd_ctx; // 将fd_ctx存到data的指针中

    // 注册事件
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    // 待执行IO事件数加1
    ++m_pendingEventCount;
    // 找到这个fd的event事件对应的EventContext，对其中的scheduler, cb, fiber进行赋值
    fd_ctx->events = (Event)(fd_ctx->events | event);// 将 fd_ctx 的注册事件更新
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);// 获得对应事件的 EventContext
    // EventContext的成员应该都为空
    SY_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);

    // 获得当前调度器
    event_ctx.scheduler = Scheduler::GetThis();
    // 如果有回调就执行回调，没有就执行该协程
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        SY_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                      ,"state=" << event_ctx.fiber->getState());
    }
    return 0;
}

// 删除事件：不会触发事件
bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    // 拿到 fd 对应的 FdContext
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 若没有要删除的事件
    if (!(fd_ctx->events & event)) {
        return false;
    }

    // 将事件从注册事件中删除
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;// 若还有事件则是修改，若没事件了则删除
    epoll_event epevent;
    epevent.events = EPOLLET | new_events; // 水平触发模式，新的注册事件
    epevent.data.ptr = fd_ctx;// ptr 关联 fd_ctx

    // 注册事件
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    // 待执行事件数减1
    --m_pendingEventCount;
    // 更新事件
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);// 拿到对应事件的EventContext
    fd_ctx->resetContext(event_ctx);// 重置EventContext
    return true;
}

// 取消事件：如果该事件被注册过回调，那就触发一次回调事件
bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(SY_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    // 删除事件
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    // 删除之前触发一次事件
    fd_ctx->triggerEvent(event);
    // 活跃事件数减1
    --m_pendingEventCount;
    return true;
}

// 取消所有事件：所有被注册的回调事件在cancel之前都会被执行一次
bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) {
        return false;
    }

    // 删除全部事件
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    // 触发全部已注册的事件
    // 有读事件执行读事件
    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    // 有写事件执行写事件
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    SY_ASSERT(fd_ctx->events == 0);
    return true;
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}
