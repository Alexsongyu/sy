#ifndef __SY_MUTEX_H__
#define __SY_MUTEX_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include <list>

#include "noncopyable.h"
#include "fiber.h"

namespace sy {

// 信号量
class Semaphore : Noncopyable {
public:
    Semaphore(uint32_t count = 0);

    ~Semaphore();

    void wait();

    void notify();
private:
    sem_t m_semaphore; // 信号量，它本质上是一个长整型的数
};

// 局部锁的模板实现：用来分装互斥量，自旋锁，原子锁
template<class T>
struct ScopedLockImpl {
public:
    // 构造函数时自动 lock
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }

    // 析构函数时自动unlock
    ~ScopedLockImpl() {
        unlock();
    }

    // 加锁
    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    // 解锁
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked; // 是否已上锁
};

// 局部读锁模板实现：用于封装读锁
template<class T>
struct ReadScopedLockImpl {
public:
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl() {
        unlock();
    }


    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

// 局部写锁模板实现：用于封装写锁
template<class T>
struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

// 互斥量（互斥锁）
class Mutex : Noncopyable {
public: 
    // 局部锁
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    // 当一个线程调用pthread_mutex_lock()时，如果当前该互斥锁没有被其它线程持有，则该线程会获得该互斥锁，并将其标记为已被持有
    // 如果该互斥锁已经被其它线程持有，则当前线程会被阻塞，直到该互斥锁被释放并重新尝试加锁
    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    // 当一个线程调用pthread_mutex_unlock()时，该互斥锁将被标记为未被持有，并且如果有其它线程正在等待该锁，则其中一个线程将被唤醒以继续执行
    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

// 空锁(用于调试！！！)
class NullMutex : Noncopyable{
public:
    // 局部锁
    typedef ScopedLockImpl<NullMutex> Lock;

    NullMutex() {}

    ~NullMutex() {}

    void lock() {}

    void unlock() {}
};

// 读写互斥量（读写互斥锁）
class RWMutex : Noncopyable{
public:

    //局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    //局部写锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    // 读写锁是一种同步机制，用于在多线程环境下对共享资源进行访问控制。
    // 与互斥锁不同，读写锁允许多个线程同时读取共享资源，但只允许一个线程写入共享资源。这样可以提高程序的性能和效率，但需要注意避免读写锁死锁等问题
    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    
    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    // 上读锁：用于获取读取锁(pthread_rwlock_t)上的共享读取访问权限。
    // 它允许多个线程同时读取共享资源，但不能写入它。
    // 调用此函数时，如果另一个线程已经持有写入锁，则该线程将被阻塞，直到写入锁被释放。
    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    // 上写锁：用于获取写入锁(pthread_rwlock_t)上的排他写访问权限。
    // 它阻止其他线程读取或写入共享资源，直到该线程释放写入锁。
    // 如果有其他线程已经持有读取或写入锁，则调用此函数的线程将被阻塞，直到所有的读取和写入锁都被释放。
    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    // 解锁：用于释放读取或写入锁。它允许其他线程获取相应的锁来访问共享资源。
    // 如果当前线程没有持有读取或写入锁，则调用pthread_rwlock_unlock将导致未定义的行为。
    // 此外，如果已经销毁了读写锁，则再次调用pthread_rwlock_unlock也会导致未定义的行为。
    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock; // 读写锁
};

// 空读写锁(用于调试！！！)
class NullRWMutex : Noncopyable {
public:
    // 局部读锁
    typedef ReadScopedLockImpl<NullMutex> ReadLock;
    // 局部写锁
    typedef WriteScopedLockImpl<NullMutex> WriteLock;

    NullRWMutex() {}

    ~NullRWMutex() {}

    void rdlock() {}

    void wrlock() {}

    void unlock() {}
};

// 自旋锁
class Spinlock : Noncopyable {
public:
    // 局部锁
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    // 用于获取自旋锁(pthread_spinlock_t)上的排他访问权限。
    // 与mutex不同，自旋锁在获取锁时不会使线程进入睡眠状态，而是进行忙等待，即不断地检查锁状态是否可用，如果不可用则一直循环等待，直到锁可用。
    // 当锁被其他线程持有时，调用pthread_spin_lock()的线程将在自旋等待中消耗CPU时间，直到锁被释放并获取到锁。
    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    // 用于释放自旋锁(pthread_spinlock_t)。调用该函数可以使其他线程获取相应的锁来访问共享资源。
    // 与mutex不同，自旋锁在释放锁时并不会导致线程进入睡眠状态，而是立即释放锁并允许等待获取锁的线程快速地获取锁来访问共享资源，从而避免了线程进入和退出睡眠状态的额外开销。
    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

// 原子锁
class CASLock : Noncopyable {
public:

    typedef ScopedLockImpl<CASLock> Lock;

    CASLock() {
        m_mutex.clear();
    }

    ~CASLock() {
    }

    void lock() {
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    // 原子状态，可以用于实现线程间同步和互斥。
    // volatile关键字表示该变量可能会被异步修改，因此编译器不会对其进行优化，而是每次都从内存中读取该变量的值。
    volatile std::atomic_flag m_mutex;
};

class Scheduler;
class FiberSemaphore : Noncopyable {
public:
    typedef Spinlock MutexType;

    FiberSemaphore(size_t initial_concurrency = 0);
    ~FiberSemaphore();

    bool tryWait();
    void wait();
    void notify();

    size_t getConcurrency() const { return m_concurrency;}
    void reset() { m_concurrency = 0;}
private:
    MutexType m_mutex;
    std::list<std::pair<Scheduler*, Fiber::ptr> > m_waiters;
    size_t m_concurrency;
};



}

#endif
