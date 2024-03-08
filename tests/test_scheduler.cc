#include "sy/sy.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    SY_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    // 循环将test_fiber加入到任务队列中，并且指定第一个拿到该任务的线程一直执行
    if(--s_count >= 0) {
        sy::Scheduler::GetThis()->schedule(&test_fiber, sy::GetThreadId());
    }
}

int main(int argc, char** argv) {
    SY_LOG_INFO(g_logger) << "main";
    sy::Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    SY_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    SY_LOG_INFO(g_logger) << "over";
    return 0;
}

// 设置2个线程， 并且将use_caller设为false， 设置名称为"work"， 指定线程
// 这里可以看到有3个线程 1684 1685 1686
// 1684为调度线程, 1685和1686为执行任务的线程
// 可以看到任务都是在1686线程上执行的，因为在shceduler()时指定了任务在第一个拿到该任务的线程上一直执行
// 1684    main    0       [INFO]  [root]  tests/test_scheduler.cc:20      main start
// 1684    main    0       [INFO]  [root]  tests/test_scheduler.cc:23      schedule
// 1686    work_1  4       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---5
// 1686    work_1  4       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---4
// 1686    work_1  4       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---3
// 1686    work_1  4       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---2
// 1686    work_1  4       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---1
// 1685    work_0  0       [INFO]  [system]        sy/scheduler.cc:237  idle_fiber term
// 1686    work_1  0       [INFO]  [system]        sy/scheduler.cc:237  idle_fiber term
// 1684    main    0       [INFO]  [root]  tests/test_scheduler.cc:27      main end

// 设置2个线程， 并且将use_caller设为true， 设置名称为"work"，不指定线程
// 这里可以看到有2个线程 2841 2842
// 2841为调度线程，他也将自己纳入调度器中执行任务
// 2841    work    0       [INFO]  [root]  tests/test_scheduler.cc:20      main start
// 2841    work    0       [INFO]  [root]  tests/test_scheduler.cc:23      schedule
// 2842    work_0  4       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---5
// 2841    work    6       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---4
// 2842    work_0  4       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---3
// 2841    work    6       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---2
// 2842    work_0  4       [INFO]  [root]  tests/test_scheduler.cc:7       ---test in fiber---1
// 2842    work_0  0       [INFO]  [system]        sy/scheduler.cc:237  idle_fiber term
// 2841    work    1       [INFO]  [system]        sy/scheduler.cc:237  idle_fiber term
// 2841    work    0       [INFO]  [root]  tests/test_scheduler.cc:27      main end