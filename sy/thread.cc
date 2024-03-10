#include "thread.h"
#include "log.h"
#include "util.h"

namespace sy {

// static thread_local用于定义静态线程本地存储变量。
// 当一个变量被声明为static thread_local时，它会在每个线程中拥有自己独立的静态实例，并且对其他线程不可见。这使得变量可以跨越多个函数调用和代码块，在整个程序运行期间保持其状态和值不变。
// 需要注意的是，由于静态线程本地存储变量是线程特定的，因此它们的初始化和销毁时机也与普通静态变量不同。在每个线程首次访问该变量时会进行初始化，在线程结束时才会进行销毁，而不是在程序启动或运行期间进行一次性初始化或销毁。
static thread_local Thread* t_thread = nullptr; // 指向当前线程 
static thread_local std::string t_thread_name = "UNKNOW"; // 指向线程名称

static sy::Logger::ptr g_logger = SY_LOG_NAME("system");

// 每个线程都有两个线程局部变量，一个用于存储当前线程的Thread指针，另一个存储线程名称
// 通过Thread::GetThis()可以获取当前的线程指针，Thread::GetName()获取当前的线程名称
Thread* Thread::GetThis() {
    return t_thread;
}
const std::string& Thread::GetName() {
    return t_thread_name;
}

// 设置当前线程名称
void Thread::SetName(const std::string& name) {
    if(name.empty()) {
        return;
    }
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

// 构造函数初始化 cb 线程执行函数，name 线程名称，并创建新线程
Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    // 创建新线程，并将其与Thread::run方法关联，创建的新线程对象this作为参数传给run方法
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        SY_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    // 在出构造函数之前，确保线程先运行起来, 保证能够初始化id。
    m_semaphore.wait();
}

// 析构：如果m_thread存在，则释放与线程关联的资源，确保线程终止。这样可以避免在线程退出时挂起，出现内存泄漏
Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);
    }
}

// 等待线程执行完成并获取其返回值，同时回收线程所使用的资源
void Thread::join() {
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            SY_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

// 线程执行
void* Thread::run(void* arg) {
    // 拿到新创建的Thread对象
    Thread* thread = (Thread*)arg;
    // 更新当前线程
    t_thread = thread;
    t_thread_name = thread->m_name;
    // 设置当前线程的id
    // 只有进了run方法才是新线程在执行，创建时是由主线程完成的，threadId为主线程的
    thread->m_id = sy::GetThreadId();
    // 设置线程名称
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    // pthread_creat时返回引用 防止函数有智能指针,
    std::function<void()> cb;
    cb.swap(thread->m_cb);

    // 在出构造函数之前，确保线程先跑起来，保证能够初始化id
    thread->m_semaphore.notify();

    cb();
    return 0;
}

}
