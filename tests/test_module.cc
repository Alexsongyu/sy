#include "sy/module.h"
#include "sy/singleton.h"
#include <iostream>
#include "sy/log.h"
#include "sy/db/redis.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

class A {
public:
    A() {
        std::cout << "A::A " << this << std::endl;
    }

    ~A() {
        std::cout << "A::~A " << this << std::endl;
    }

};

class MyModule : public sy::RockModule {
public:
    MyModule()
        :RockModule("hello", "1.0", "") {
        //sy::Singleton<A>::GetInstance();
    }

    bool onLoad() override {
        sy::Singleton<A>::GetInstance();
        std::cout << "-----------onLoad------------" << std::endl;
        return true;
    }

    bool onUnload() override {
        sy::Singleton<A>::GetInstance();
        std::cout << "-----------onUnload------------" << std::endl;
        return true;
    }

    bool onServerReady() {
        registerService("rock", "sy.top", "blog");
        auto rpy = sy::RedisUtil::Cmd("local", "get abc");
        if(!rpy) {
            SY_LOG_ERROR(g_logger) << "redis cmd get abc error";
        } else {
            SY_LOG_ERROR(g_logger) << "redis get abc: "
                << (rpy->str ? rpy->str : "(null)");
        }
        return true;
    }

    bool handleRockRequest(sy::RockRequest::ptr request
                        ,sy::RockResponse::ptr response
                        ,sy::RockStream::ptr stream) {
        //SY_LOG_INFO(g_logger) << "handleRockRequest " << request->toString();
        //sleep(1);
        response->setResult(0);
        response->setResultStr("ok");
        response->setBody("echo: " + request->getBody());

        usleep(100 * 1000);
        auto addr = stream->getLocalAddressString();
        if(addr.find("8061") != std::string::npos) {
            if(rand() % 100 < 50) {
                usleep(10 * 1000);
            } else if(rand() % 100 < 10) {
                response->setResult(-1000);
            }
        } else {
            //if(rand() % 100 < 25) {
            //    usleep(10 * 1000);
            //} else if(rand() % 100 < 10) {
            //    response->setResult(-1000);
            //}
        }
        return true;
        //return rand() % 100 < 90;
    }

    bool handleRockNotify(sy::RockNotify::ptr notify 
                        ,sy::RockStream::ptr stream) {
        SY_LOG_INFO(g_logger) << "handleRockNotify " << notify->toString();
        return true;
    }

};

extern "C" {

sy::Module* CreateModule() {
    sy::Singleton<A>::GetInstance();
    std::cout << "=============CreateModule=================" << std::endl;
    return new MyModule;
}

void DestoryModule(sy::Module* ptr) {
    std::cout << "=============DestoryModule=================" << std::endl;
    delete ptr;
}

}
