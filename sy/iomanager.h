#ifndef __SY_IOMANAGER_H__
#define __SY_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace sy {

// IO协程调度器
// 继承TimerManager类的所有方法，从而可以管理定时器
class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    // IO事件
    // 这里只关心socket fd的读和写事件，其他epoll事件会归类到这两类事件中
    enum Event {
        // 无事件
        NONE    = 0x0,
        // 读事件(EPOLLIN)
        READ    = 0x1,
        // 写事件(EPOLLOUT)
        WRITE   = 0x4,
    };
private:
    // socket fd上下文类
    // 每个socket fd都对应一个FdContext，包括fd的值，fd上的事件，以及fd的读写事件上下文
    struct FdContext {
        typedef Mutex MutexType;
        // 事件上下文类
        // fd的每个事件都有一个事件上下文，保存这个事件的回调函数以及执行回调函数的调度器
        struct EventContext {
            // 执行事件的调度器
            Scheduler* scheduler = nullptr;
            // 事件协程
            Fiber::ptr fiber;
            /// 事件执行的回调函数
            std::function<void()> cb;
        };

        // 获取事件上下文：event 事件类型，返回对应事件的上下文
        EventContext& getContext(Event event);

        // 重置事件上下文：ctx 待重置的事件上下文对象
        void resetContext(EventContext& ctx);

        // 触发事件：根据事件类型调用对应上下文结构中的调度器去调度回调协程或回调函数
        void triggerEvent(Event event);

        // 读事件上下文
        EventContext read;
        // 写事件上下文
        EventContext write;
        // 事件关联的句柄
        int fd = 0;
        // 该fd添加了哪些事件的回调函数，或者说该fd已经注册的事件
        Event events = NONE;
        // 事件的Mutex
        MutexType mutex;
    };

public:
    // 构造函数：threads 线程数量，use_caller 是否将调用线程包含进去，name 调度器的名称
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    // 析构函数
    ~IOManager();

    // 添加事件:添加成功返回0,失败返回-1
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    // 删除事件:不会触发事件
    bool delEvent(int fd, Event event);

    // 取消事件:如果事件存在则触发事件
    bool cancelEvent(int fd, Event event);

    // 取消所有事件
    bool cancelAll(int fd);

    // 返回当前的IOManager
    static IOManager* GetThis();
protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;

    // 重置socket句柄上下文的容器大小
    void contextResize(size_t size);

    // 判断是否可以停止:timeout 最近要出发的定时器事件间隔
    bool stopping(uint64_t& timeout);
private:
    // epoll 文件句柄
    int m_epfd = 0;
    // pipe 文件句柄，fd[0]读端，fd[1]写端
    int m_tickleFds[2];
    // 当前等待执行的IO事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    // IOManager的Mutex
    RWMutexType m_mutex;
    // socket事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};

}

#endif
