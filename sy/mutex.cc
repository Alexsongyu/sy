// 信号量实现
#include "mutex.h"
#include "macro.h"
#include "scheduler.h"

namespace sy {
// 对信号量的初始化、销毁、获取和释放，用于线程间的同步和互斥控制

// 构造函数初始化信号量，count 信号量初始值的大小
// 调用 sem_init 函数初始化信号量，如果初始化失败则抛出逻辑错误异常
Semaphore::Semaphore(uint32_t count) {
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

// 析构函数销毁信号量
// 只有在确保没有任何线程或进程正在使用该信号量时，才应该调用析构函数。否则，可能会导致未定义的行为
Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore);
}

// 获取信号量，即等待信号量的值大于0
void Semaphore::wait() {
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

// 释放信号量，增加信号量的值
void Semaphore::notify() {
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}

FiberSemaphore::FiberSemaphore(size_t initial_concurrency)
    :m_concurrency(initial_concurrency) {
}

FiberSemaphore::~FiberSemaphore() {
    SY_ASSERT(m_waiters.empty());
}

bool FiberSemaphore::tryWait() {
    SY_ASSERT(Scheduler::GetThis());
    {
        MutexType::Lock lock(m_mutex);
        if(m_concurrency > 0u) {
            --m_concurrency;
            return true;
        }
        return false;
    }
}

void FiberSemaphore::wait() {
    SY_ASSERT(Scheduler::GetThis());
    {
        MutexType::Lock lock(m_mutex);
        if(m_concurrency > 0u) {
            --m_concurrency;
            return;
        }
        m_waiters.push_back(std::make_pair(Scheduler::GetThis(), Fiber::GetThis()));
    }
    Fiber::YieldToHold();
}

void FiberSemaphore::notify() {
    MutexType::Lock lock(m_mutex);
    if(!m_waiters.empty()) {
        auto next = m_waiters.front();
        m_waiters.pop_front();
        next.first->schedule(next.second);
    } else {
        ++m_concurrency;
    }
}

}
