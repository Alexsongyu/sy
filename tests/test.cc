#include <iostream>
#include "sy/log.h"
#include "sy/util.h"

int main(int argc, char** argv) {
    sy::Logger::ptr logger(new sy::Logger);
    logger->addAppender(sy::LogAppender::ptr(new sy::StdoutLogAppender));

    sy::FileLogAppender::ptr file_appender(new sy::FileLogAppender("./log.txt"));
    sy::LogFormatter::ptr fmt(new sy::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sy::LogLevel::ERROR);

    logger->addAppender(file_appender);

    //sy::LogEvent::ptr event(new sy::LogEvent(__FILE__, __LINE__, 0, sy::GetThreadId(), sy::GetFiberId(), time(0)));
    //event->getSS() << "hello sy log";
    //logger->log(sy::LogLevel::DEBUG, event);
    std::cout << "hello sy log" << std::endl;

    SY_LOG_INFO(logger) << "test macro";
    SY_LOG_ERROR(logger) << "test macro error";

    SY_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    auto l = sy::LoggerMgr::GetInstance()->getLogger("xx");
    SY_LOG_INFO(l) << "xxx";
    return 0;
}
