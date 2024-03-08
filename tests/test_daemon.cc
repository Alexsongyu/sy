#include "sy/daemon.h"
#include "sy/iomanager.h"
#include "sy/log.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

sy::Timer::ptr timer;
int server_main(int argc, char** argv) {
    SY_LOG_INFO(g_logger) << sy::ProcessInfoMgr::GetInstance()->toString();
    sy::IOManager iom(1);
    timer = iom.addTimer(1000, [](){
            SY_LOG_INFO(g_logger) << "onTimer";
            static int count = 0;
            if(++count > 10) {
                exit(1);
            }
    }, true);
    return 0;
}

int main(int argc, char** argv) {
    return sy::start_daemon(argc, argv, server_main, argc != 1);
}
