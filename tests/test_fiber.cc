#include "sy/sy.h"
#include "sy/fiber.h"
#include <string>
#include <vector>

sy::Logger::ptr g_logger = SY_LOG_ROOT();

void run_in_fiber2() {
    SY_LOG_INFO(g_logger) << "run_in_fiber2 begin";
    SY_LOG_INFO(g_logger) << "run_in_fiber2 end";
}

void test_fiber() {
    SY_LOG_INFO(g_logger) << "main begin -1";
    {
        sy::Fiber::GetThis(); //获得主协程 
        SY_LOG_INFO(g_logger) << "main begin";
        sy::Fiber::ptr fiber(new sy::Fiber(run_in_fiber)); //获得子协程，该协程与run_in_fiber方法绑定
        fiber->resume(); //从当前主协程切换到子协程执行任务
        SY_LOG_INFO(g_logger) << "main after resume";
        fiber->resume();
        SY_LOG_INFO(g_logger) << "main after end";
        fiber->resume();
    }
    SY_LOG_INFO(g_logger) << "main after end2";
}

// 在main中创建了3个线程执行test_fiber函数，每个线程在创建时都绑定了各自的Thread::run方法，在run方法中执行test_fiber
// run方法执行时会初始化线程信息：初始化t_thread，线程名称，线程id
int main(int argc, char** argv) {
    sy::Thread::SetName("main");

    std::vector<sy::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(sy::Thread::ptr(
                    new sy::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}
