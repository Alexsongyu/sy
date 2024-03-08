#include "sy/sy.h"
#include <unistd.h>

sy::Logger::ptr g_logger = SY_LOG_ROOT();

int count = 0;
//sy::RWMutex s_mutex;
sy::Mutex s_mutex;

void fun1() {
    SY_LOG_INFO(g_logger) << "name: " << sy::Thread::GetName()
                             << " this.name: " << sy::Thread::GetThis()->getName()
                             << " id: " << sy::GetThreadId()
                             << " this.id: " << sy::Thread::GetThis()->getId();

    for(int i = 0; i < 100000; ++i) {
        //sy::RWMutex::WriteLock lock(s_mutex);
        sy::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        SY_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        SY_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv) {
    SY_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/sy/test/sy/bin/conf/log2.yml");
    sy::Config::LoadFromYaml(root);

    std::vector<sy::Thread::ptr> thrs;
    for(int i = 0; i < 1; ++i) {
        sy::Thread::ptr thr(new sy::Thread(&fun2, "name_" + std::to_string(i * 2)));
        //sy::Thread::ptr thr2(new sy::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        //thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    SY_LOG_INFO(g_logger) << "thread test end";
    SY_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}
