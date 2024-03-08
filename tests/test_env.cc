#include "sy/env.h"
#include <unistd.h>
#include <iostream>
#include <fstream>

struct A {
    A() {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        for(size_t i = 0; i < content.size(); ++i) {
            std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
        }
    }
};

A a;

int main(int argc, char** argv) {
    std::cout << "argc=" << argc << std::endl;
    sy::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    sy::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    sy::EnvMgr::GetInstance()->addHelp("p", "print help");
    if(!sy::EnvMgr::GetInstance()->init(argc, argv)) {
        sy::EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << sy::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << sy::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << sy::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << sy::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    std::cout << "set env " << sy::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << sy::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    if(sy::EnvMgr::GetInstance()->has("p")) {
        sy::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}
