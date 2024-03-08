#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace sy {

static sy::Logger::ptr g_logger = SY_LOG_NAME("system");

// 当前线程的调度器，同一个调度器下的所有线程共享同一个实例
static thread_local Scheduler *t_scheduler = nullptr;
// 当前线程的调度协程，每个线程都独有一份
static thread_local Fiber *t_scheduler_fiber = nullptr;

// 初始化调度器
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name) {
    SY_ASSERT(threads > 0); // 确定线程数量要正确

    // 使用 caller 线程执行调度任务
    if(use_caller) { 
        --threads; // 线程数量-1
        sy::Fiber::GetThis(); // 获得主协程
        SY_ASSERT(GetThis() == nullptr);
        t_scheduler = this; // 设置当前协程调度器

        //caller线程的主协程不会被线程的调度协程run进行调度，而且，线程的调度协程停止时，应该返回caller线程的主协程
        // 在user caller情况下，把caller线程的主协程暂时保存起来，等调度协程结束时，再resume caller协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));

        sy::Thread::SetName(m_name); // 设置线程名称

        // 设置当前线程的主协程为m_rootFiber
        // 这里的m_rootFiber是该线程的主协程（执行run任务的协程），只有默认构造出来的fiber才是主协程
        t_scheduler_fiber = m_rootFiber.get();

        // 获得当前线程id
        m_rootThread = sy::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } 
    // 不将当前线程纳入调度器
    else { 
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    SY_LOG_DEBUG(g_logger) << "Scheduler::~Scheduler()";
    SY_ASSERT(m_stopping); // 必须达到停止条件
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

// 启动调度器
void Scheduler::start() {
    SY_LOG_DEBUG(g_logger) << "start";
    MutexType::Lock lock(m_mutex);
    // 已经启动了
    if(!m_stopping) {
        SY_LOG_ERROR(g_logger) << "Scheduler is stopped";
        return;
    }
    // 线程池为空
    SY_ASSERT(m_threads.empty());
    // 创建线程池
    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
         // 线程执行 run() 任务
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

// 停止调度器
void Scheduler::stop() {
    SY_LOG_DEBUG(g_logger) << "stop";
    if (stopping()) {
        return;
    }
    m_stopping = true;

    // 如果use caller，那只能由caller线程发起stop
    if (m_useCaller) {
        SY_ASSERT(GetThis() == this);
    } else {
        SY_ASSERT(GetThis() != this);
    }

    for (size_t i = 0; i < m_threadCount; i++) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    // 在use caller情况下，调度器协程结束时，应该返回caller协程
    if (m_rootFiber) {
        m_rootFiber->resume();
        SY_LOG_DEBUG(g_logger) << "m_rootFiber end";
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for (auto &i : thrs) {
        i->join();
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

// 调度协程
void Scheduler::run() {
    SY_LOG_DEBUG(g_logger) << m_name << " run";
    // hook
    set_hook_enable(true);
     // 设置当前调度器
    setThis();

    // 在非caller线程里，调度协程就是调度线程的主协程
    if(sy::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = sy::Fiber::GetThis().get();
    }

    SY_LOG_DEBUG(g_logger) << "new idle_fiber";

    // 定义dile_fiber，当任务队列中的任务执行完之后，执行idle()
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    ScheduleTask task;
    while(true) {
        task.reset();
        bool tickle_me = false;
        {
            // 从任务队列中拿fiber和cb
            MutexType::Lock lock(m_mutex);
            auto it = m_tasks.begin();
            // 遍历所有调度任务
            while (it != m_tasks.end()) {
                if (it->thread != -1 && it->thread != sy::GetThreadId()) {
                    // 指定了调度线程，但不是在当前线程上调度，标记一下需要通知其他线程进行调度，然后跳过这个任务，继续下一个
                    ++it;
                    tickle_me = true;
                    continue;
                }

                // 找到一个未指定线程，或是指定了当前线程的任务
                SY_ASSERT(it->fiber || it->cb);
                if (it->fiber) {
                    // 任务队列时的协程一定是READY状态，谁会把RUNNING或TERM状态的协程加入调度呢？
                    SY_ASSERT(it->fiber->getState() == Fiber::READY);
                }
                // 当前调度线程找到一个任务，准备开始调度，将其从任务队列中剔除，活动线程数加1
                task = *it;
                m_tasks.erase(it++);
                ++m_activeThreadCount;
                break;
            }
            // 当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
            tickle_me |= (it != m_tasks.end());
        }

        // 取到任务tickle一下
        if(tickle_me) {
            tickle();
        }

        // 如果任务是fiber，并且任务处于可执行状态
        if (task.fiber) {
            // resume协程，resume返回时，协程要么执行完了，要么半路yield了，总之这个任务就算完成了，活跃线程数减一
            task.fiber->resume();
            --m_activeThreadCount;
            task.reset();
        } else if (task.cb) {
            if (cb_fiber) {
                cb_fiber->reset(task.cb);
            } else {
                cb_fiber.reset(new Fiber(task.cb));
            }
            task.reset();
            cb_fiber->resume();
            --m_activeThreadCount;
            cb_fiber.reset();
        } else {
            // 进到这个分支情况一定是任务队列空了，调度idle协程即可
            if (idle_fiber->getState() == Fiber::TERM) {
                // 如果调度器没有调度任务，那么idle协程会不停地resume/yield，不会结束，如果idle协程结束了，那一定是调度器停止了
                SY_LOG_DEBUG(g_logger) << "idle fiber term";
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->resume();
            --m_idleThreadCount;
        }
    }
    SY_LOG_DEBUG(g_logger) << "Scheduler::run() exit";
}

void Scheduler::tickle() {
    SY_LOG_INFO(g_logger) << "tickle";
}

// 判断停止条件
bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    // 当正在停止 && 任务队列为空 && 活跃的线程数量为0
    return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SY_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        sy::Fiber::GetThis()->yield();
    }
}

}
