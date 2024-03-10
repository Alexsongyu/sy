#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace sy {

static Logger::ptr g_logger = SY_LOG_NAME("system");

// 全局静态变量：多线程共享
// 用于生成协程id
static std::atomic<uint64_t> s_fiber_id {0};
// 用于统计当前的协程数
static std::atomic<uint64_t> s_fiber_count {0};

// 线程局部变量
// 当前线程正在运行的协程：初始化时，指向线程主协程
static thread_local Fiber *t_fiber = nullptr;
// 当前线程的主协程：初始化时，指向线程主协程。切换到了主协程就相当于切换到了主线程中运行。
static thread_local Fiber::ptr t_threadFiber = nullptr;

// 约定协程栈大小，可通过配置文件获取，默认128k
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

// malloc栈内存分配器：创建/释放协程运行栈
class MallocStackAllocator {
public:
    static void* Alloc(size_t size) { return malloc(size); }
    static void Dealloc(void* vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

// 无参构造函数只用于创建线程的第一个协程，也就是线程主函数对应的协程，这个协程只能由GetThis()方法调用，所以定义成私有方法
Fiber::Fiber() {
    SetThis(this);
    m_state = RUNNING;
    
    // 获取当前协程的上下文信息保存到m_ctx中
    if (getcontext(&m_ctx)) {
        SY_ASSERT2(false, "getcontext");
    }
 
    ++s_fiber_count;
    m_id = s_fiber_id++; // 协程id从0开始，用完加1
 
    SY_LOG_DEBUG(g_logger) << "Fiber::Fiber() main id = " << m_id;
}

void Fiber::SetThis(Fiber *f) {
    t_fiber = f;
}

// 获取当前协程，同时充当初始化当前线程主协程的作用，这个函数在使用协程之前要调用一下
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    Fiber::ptr main_fiber(new Fiber);
    SY_ASSERT(t_fiber == main_fiber.get());
    t_thread_fiber = main_fiber;
    return t_fiber->shared_from_this();
}

// 有参构造函数，用于创建其他协程（用户协程）
// 增加m_runInScheduler成员，表示当前协程是否参与调度器调度
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler)
    : m_id(s_fiber_id++)
    , m_cb(cb) 
    , m_runInScheduler(run_in_scheduler){
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    m_stack     = StackAllocator::Alloc(m_stacksize);
 
    if (getcontext(&m_ctx)) {
        SY_ASSERT2(false, "getcontext");
    }
 
    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
 
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
 
    SY_LOG_DEBUG(g_logger) << "Fiber::Fiber() id = " << m_id;
}

// 线程的主协程析构时需要特殊处理，因为主协程没有分配栈和cb
Fiber::~Fiber() {
    SY_LOG_DEBUG(g_logger) << "Fiber::~Fiber() id = " << m_id;
    --s_fiber_count;
    // 有栈，说明是子协程，需要确保子协程一定是结束状态
    if(m_stack) {
        SY_ASSERT(m_state == TERM);
        // 释放运行栈
        StackAllocator::Dealloc(m_stack, m_stacksize);
        SY_LOG_DEBUG(g_logger) << "dealloc stack, id = " << m_id;
    } 
    // 没有栈，说明是线程的主协程
    else {
        // 主协程的释放要保证：主协程没有任务cb，并且主协程当前正在运行
        SY_ASSERT(!m_cb);
        SY_ASSERT(m_state == RUNNING);

        //若当前协程为主协程，将当前协程置为空
        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
}

// 这里强制只有TERM状态的协程才可以重置，但其实刚创建好但没执行过的协程也应该允许重置的
// 重置协程就是重复利用已结束的协程，复用其栈空间，创建新协程
void Fiber::reset(std::function<void()> cb) {
    SY_ASSERT(m_stack);
    SY_ASSERT(m_state == TERM);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        SY_ASSERT2(false, "getcontext");
    }
 
    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
 
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = READY;
}

// 将当前协程切到到执行状态
// 当前协程和正在运行的协程进行交换，前者状态变为RUNNING，后者状态变为READY
void Fiber::resume() {
    SY_ASSERT(m_state != TERM && m_state != RUNNING);
    SetThis(this);
    m_state = RUNNING;
 
    // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
    if (m_runInScheduler) {
        if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
            SY_ASSERT2(false, "swapcontext");
        }
    } else {
        if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx)) {
            SY_ASSERT2(false, "swapcontext");
        }
    }
}
 
// 当前协程让出执行权
// 当前协程与上次resume时退到后台的协程进行交换，前者状态变为READY，后者状态变为RUNNING
void Fiber::yield() {
    /// 协程运行完之后会自动yield一次，用于回到主协程，此时状态已为结束状态
    SY_ASSERT(m_state == RUNNING || m_state == TERM);
    SetThis(t_thread_fiber.get());
    if (m_state != TERM) {
        m_state = READY;
    }
 
    // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
    if (m_runInScheduler) {
        if (swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
            SY_ASSERT2(false, "swapcontext");
        }
    } else {
        if (swapcontext(&m_ctx, &(t_thread_fiber->m_ctx))) {
            SY_ASSERT2(false, "swapcontext");
        }
    }
}

// 协程入口函数
void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis(); // GetThis()的shared_from_this()方法让引用计数加1
    SY_ASSERT(cur);
 
    cur->m_cb(); // 这里真正执行协程的入口函数
    cur->m_cb    = nullptr;
    cur->m_state = TERM;
 
    auto raw_ptr = cur.get(); // 手动让t_fiber的引用计数减1
    cur.reset();
    raw_ptr->yield(); // 协程结束时自动yield，以回到主协程
}

}
