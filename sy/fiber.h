#ifndef __SY_FIBER_H__
#define __SY_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace sy {

class Scheduler;

// 协程类
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    // 协程的三种状态
    enum State {
        // 就绪态：刚创建或者yield之后的状态
        READY,
        // 运行态
        RUNNING,
        // 结束：协程的回调函数执行完之后的状态
        TERM,
    };
private:
    // 无参构造函数只用于创建线程的第一个协程，也就是线程主函数对应的协程
    // 这个协程只能由GetThis()方法调用，所以定义为私有
    Fiber();

public:
    // 构造子协程（用户协程）：初始化子协程的ucontext_t上下文和栈空间
    // cb 协程入口函数，stacksize 协程栈大小，run_in_scheduler 本协程是否参与调度器调度，默认为true
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);

    // 释放协程运行栈
    ~Fiber();

    // 重置协程入口函数,并设置协程状态
    void reset(std::function<void()> cb);

    // 将当前线程切换到执行状态
    // 当前协程和正在运行的协程进行交换，前者状态变为RUNNING，后者状态变为READY
    void resume();

    // 当前协程让出执行权
    // 当前协程与上次resume时退到后台的协程进行交换，前者状态变为READY，后者状态变为RUNNING
    void yield();

    // 获取协程id
    uint64_t getId() const { return m_id;}

    // 获取协程状态
    State getState() const { return m_state;}
public:

    // 设设置当前正在运行的协程，即设置线程局部变量t_fiber的值
    static void SetThis(Fiber* f);

    // 返回当前线程正在执行的协程
    // 如果当前线程还未创建协程，则创建线程的第一个协程，且该协程为当前线程的主协程，其他协程都通过这个协程来调度
    // 也就是说，其他协程结束时,都要切回到主协程，由主协程重新选择新的协程进行resume
    // 线程如果要创建协程，那么应该首先执行一下Fiber::GetThis()操作，以初始化主函数协程
    static Fiber::ptr GetThis();

    // 获取总协程数
    static uint64_t TotalFibers();

    // 协程执行入口函数，执行完成返回到线程主协程
    static void MainFunc();

    // 获取当前协程的id
    static uint64_t GetFiberId();
private:
    // 协程id
    uint64_t m_id = 0;
    // 协程运行栈大小
    uint32_t m_stacksize = 0;
    // 协程状态
    State m_state = READY;
    // 协程上下文
    ucontext_t m_ctx;
    // 协程运行栈指针
    void* m_stack = nullptr;
    // 协程运行函数
    std::function<void()> m_cb;
    // 本协程是否参与调度器调度
    bool m_runInScheduler;
};

}

#endif
