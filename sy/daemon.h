#ifndef __SY_DAEMON_H__
#define __SY_DAEMON_H__

#include <unistd.h>
#include <functional>
#include "sy/singleton.h"

namespace sy {

struct ProcessInfo {
    // 父进程id
    pid_t parent_id = 0;
    // 主进程id
    pid_t main_id = 0;
    // 父进程启动时间
    uint64_t parent_start_time = 0;
    // 主进程启动时间
    uint64_t main_start_time = 0;
    // 主进程重启的次数
    uint32_t restart_count = 0;

    std::string toString() const;
};

typedef sy::Singleton<ProcessInfo> ProcessInfoMgr;

// 启动程序可以选择用守护进程的方式
// 输入：argc 参数个数，argv 参数值数组，main_cb 启动函数，is_daemon 是否守护进程的方式
int start_daemon(int argc, char** argv
                 , std::function<int(int argc, char** argv)> main_cb
                 , bool is_daemon);

}

#endif
