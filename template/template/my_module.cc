#include "my_module.h"
#include "sy/config.h"
#include "sy/log.h"

namespace name_space {

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

MyModule::MyModule()
    :sy::Module("project_name", "1.0", "") {
}

bool MyModule::onLoad() {
    SY_LOG_INFO(g_logger) << "onLoad";
    return true;
}

bool MyModule::onUnload() {
    SY_LOG_INFO(g_logger) << "onUnload";
    return true;
}

bool MyModule::onServerReady() {
    SY_LOG_INFO(g_logger) << "onServerReady";
    return true;
}

bool MyModule::onServerUp() {
    SY_LOG_INFO(g_logger) << "onServerUp";
    return true;
}

}

extern "C" {

sy::Module* CreateModule() {
    sy::Module* module = new name_space::MyModule;
    SY_LOG_INFO(name_space::g_logger) << "CreateModule " << module;
    return module;
}

void DestoryModule(sy::Module* module) {
    SY_LOG_INFO(name_space::g_logger) << "CreateModule " << module;
    delete module;
}

}
