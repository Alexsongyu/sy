// 定义宏，防止头文件被多次引用
#ifndef _SY_LOG_H // if not defined，则继续执行
#define _SY_LOG_H

#include <string>
#include <stdint.h>
#include <memory> // 指针
#include <list>
#include <sstream> // 字符串流 std::stringstream：数据转换为字符串/从字符串中提取数据
#include <fstream> // 文件流 std::ifstream/ofstream ：文件读写
#include <vector>
#include <stdarg.h>
#include <cstdarg>
#include <list>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"
#include "mutex.h"

// 定义宏记录不同级别的日志

// 使用流式方式将日志级别level的日志写入到logger
// 构造一个LogEventWrap对象，包裹包含日志器和日志事件，在对象析构时调用日志器写日志事件
// 通过使用 LogEventWrap 类对 LogEvent 智能指针进行包装，可以在宏定义的作用域结束时自动触发析构函数，立即释放 LogEvent 对象，从而确保日志事件能够正确写入到日志器中
#define SY_LOG_LEVEL(logger, level)                                                                         \
    if(level <= logger->getLevel()) \
        sy::LogEventWrap(logger, sy::LogEvent::ptr(new sy::LogEvent(logger->getName(), \
            level, __FILE__, __LINE__, sy::GetElapsed() - logger->getCreateTime(), \
            sy::GetThreadId(), sy::GetFiberId(), time(0), sy::GetThreadName()))).getLogEvent()->getSS()

#define SY_LOG_FATAL(logger) SY_LOG_LEVEL(logger, sy::LogLevel::FATAL)

#define SY_LOG_ALERT(logger) SY_LOG_LEVEL(logger, sy::LogLevel::ALERT)

#define SY_LOG_CRIT(logger) SY_LOG_LEVEL(logger, sy::LogLevel::CRIT)

#define SY_LOG_ERROR(logger) SY_LOG_LEVEL(logger, sy::LogLevel::ERROR)

#define SY_LOG_WARN(logger) SY_LOG_LEVEL(logger, sy::LogLevel::WARN)

#define SY_LOG_NOTICE(logger) SY_LOG_LEVEL(logger, sy::LogLevel::NOTICE)

#define SY_LOG_INFO(logger) SY_LOG_LEVEL(logger, sy::LogLevel::INFO)

#define SY_LOG_DEBUG(logger) SY_LOG_LEVEL(logger, sy::LogLevel::DEBUG)

// 使用C printf方式将日志级别level的日志写入到logger
// 构造一个LogEventWrap对象，包裹包含日志器和日志事件，在对象析构时调用日志器写日志事件
#define SY_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(level <= logger->getLevel()) \
        sy::LogEventWrap(logger, sy::LogEvent::ptr(new sy::LogEvent(logger->getName(), \
            level, __FILE__, __LINE__, sy::GetElapsed() - logger->getCreateTime(), \
            sy::GetThreadId(), sy::GetFiberId(), time(0), sy::GetThreadName()))).getLogEvent()->printf(fmt, __VA_ARGS__)

#define SY_LOG_FMT_FATAL(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SY_LOG_FMT_ALERT(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::ALERT, fmt, __VA_ARGS__)

#define SY_LOG_FMT_CRIT(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::CRIT, fmt, __VA_ARGS__)

#define SY_LOG_FMT_ERROR(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::ERROR, fmt, __VA_ARGS__)

#define SY_LOG_FMT_WARN(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::WARN, fmt, __VA_ARGS__)

#define SY_LOG_FMT_NOTICE(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::NOTICE, fmt, __VA_ARGS__)

#define SY_LOG_FMT_INFO(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::INFO, fmt, __VA_ARGS__)

#define SY_LOG_FMT_DEBUG(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::DEBUG, fmt, __VA_ARGS__)

// 获取主日志器
#define SY_LOG_ROOT() sy::LoggerMgr::GetInstance()->getRoot()

// 获取相应name的日志器
#define SY_LOG_NAME(name) sy::LoggerMgr::GetInstance()->getLogger(name)

// 定义类，防止别人和这里面的 类、变量、函数重名
namespace sy
{

    class Logger;
    class LoggerManager;

    // 日志级别
    class LogLevel
    {
    public:
        enum Level
        { // 枚举类型
        // 致命情况，系统不可用
        FATAL  = 0,
        // 高优先级情况，例如数据库系统崩溃
        ALERT  = 100,
        // 严重错误，例如硬盘错误
        CRIT   = 200,
        // 错误
        ERROR  = 300,
        // 警告
        WARN   = 400,
        // 正常但值得注意
        NOTICE = 500,
        // 一般信息
        INFO   = 600,
        // 调试信息
        DEBUG  = 700,
        // 未设置
        NOTSET = 800,
        };

        // 将日志级别 level 转换为文本输出
        static const char *ToString(LogLevel::Level level);
        // 将日志级别文本 str 转换为日志级别
        static LogLevel::Level FromString(const std::string &str);
    };

    // 日志事件
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        LogEvent(const std::string &logger_name, LogLevel::Level level, const char *file, int32_t line
        , int64_t elapse, uint32_t thread_id, uint64_t fiber_id, time_t time, const std::string &thread_name);

        LogLevel::Level getLevel() const { return m_level; }

        std::string getContent() const { return m_ss.str(); }

        std::string getFile() const { return m_file; }

        int32_t getLine() const { return m_line; }

        int64_t getElapse() const { return m_elapse; }

        uint32_t getThreadId() const { return m_threadId; }

        uint64_t getFiberId() const { return m_fiberId; }

        time_t getTime() const { return m_time; }

        const std::string &getThreadName() const { return m_threadName; }

        std::stringstream &getSS() { return m_ss; }

        const std::string &getLoggerName() const { return m_loggerName; }

        void printf(const char *fmt, ...);

        void vprintf(const char *fmt, va_list ap);

    private:
        // 日志级别
        LogLevel::Level m_level;
        // 日志内容，使用stringstream存储，便于流式写入日志
        std::stringstream m_ss;
        // 文件名：不可修改。变量定义加 m_ 区分局部变量
        const char *m_file = nullptr;
        // 行号
        int32_t m_line = 0;
        // 程序启动（日志器创建）到现在的耗时
        int64_t m_elapse = 0;
        // 线程 ID
        uint32_t m_threadID = 0;
        // 协程 ID
        int64_t m_fiberID = 0;
        // 时间戳
        time_t m_time;
        // 线程名称
        std::string m_threadName;
        // 日志器名称
        std::string m_loggerName;
    };

    // 定义日志输出格式
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        
        // 构造函数：日志模板pattern
        // 模板参数说明：
        // - %%m 消息
        // - %%p 日志级别
        // - %%c 日志器名称
        // - %%d 日期时间，后面可跟一对括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这里的格式字符与C语言strftime一致
        // - %%r 该日志器创建后的累计运行毫秒数
        // - %%f 文件名
        // - %%l 行号
        // - %%t 线程id
        // - %%F 协程id
        // - %%N 线程名称
        // - %%% 百分号
        // - %%T 制表符
        // - %%n 换行
        // 默认格式：%%d{%%Y-%%m-%%d %%H:%%M:%%S}%%T%%t%%T%%N%%T%%F%%T[%%p]%%T[%%c]%%T%%f:%%l%%T%%m%%n 
        // 默认格式描述：年-月-日 时:分:秒 [累计运行毫秒数] \\t 线程id \\t 线程名称 \\t 协程id \\t [日志级别] \\t [日志器名称] \\t 文件名:行号 \\t 日志消息 换行符
        LogFormatter(const std::string &pattern = "%d{%Y-%m-%d %H:%M:%S} [%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");

        // 初始化，解析格式模板，提取模板项
        void init();

        // 模板解析是否出错
        bool isError() const { return m_error; }

        // 对日志事件进行格式化：传入日志事件，返回格式化日志文本
        std::string format(LogEvent::ptr event);
        // 对日志事件进行格式化：传入日志事件，返回格式化日志流
        std::ostream &format(std::ostream &os, LogEvent::ptr event);

        // 获取pattern
        std::string getPattern() const { return m_pattern; }

    private:
        // 日志内容格式化项，虚基类，用于派生出不同的格式化项
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;

            virtual ~FormatItem(){}

            // 格式化日志到流：传入 event 返回日志输出流ostream
            virtual void format(std::ostream &os, LogEvent::ptr event) = 0;
        };

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items; // 解析后的格式模板数组
        bool m_error = false; // 判断日志格式是否错误
    };

    // 日志输出地，虚基类，用于派生出不同的LogAppender
    class LogAppender
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Spinlock MutexType;

        LogAppender(LogFormatter::ptr default_formatter); // 默认日志格式器

        virtual ~LogAppender() {} // 虚析构：确保正确释放子类对象资源

        // 写入日志
        // 将虚函数声明为纯虚函数：基类没有默认实现，需要子类实现这个纯虚函数才能被实例化
        virtual void log(LogEvent::ptr event) = 0;

        // 将日志输出目标的配置转成YAML String
        virtual std::string toYamlString() = 0;

        void setFormatter(LogFormatter::ptr val); // 设置日志格式器

        LogFormatter::ptr getFormatter();         // 获取日志格式器

    protected: // 后面的子类要用到
        // Mutex
        MutexType m_mutex;
        // 日志格式器
        LogFormatter::ptr m_formatter;
        // 默认日志格式器
        LogFormatter::ptr m_defaultFormatter;
    };

    // 向控制台输出日志
    // 重写纯虚函数log实现写入日志，重写纯虚函数toYamlString实现将日志转化为YAML格式的字符串
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<stdoutLogAppender> ptr;

        StdoutLogAppender();

        void log(LogEvent::ptr event) override; // 重载

        std::string toYamlString() override;
    };

    // 向文件输出日志
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        FileLogAppender(const std::string &filename); // 日志文件路径

        void log(LogEvent::ptr event) override;

        std::string toYamlString() override;

        bool reopen(); // 日志文件重新打开成功，返回true
    private:
        std::string m_filename;     // 文件路径
        std::ofstream m_filestream; // 文件流
        uint64_t m_lastTime = 0;    // 上次重新打开的时间，每秒reopen一次，判断文件有没有被删
        bool m_reopenError = false; // 文件打开错误标识
    };

    // 日志器
    class Logger{
        friend class LoggerManager;
    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Spinlock MutexType;

        // 构造函数 name 日志器名称
        Logger(const std::string &name="default");

        // 获取日志器名称
        const std::string &getName() const { return m_name; }

        // 获取创建时间
        const uint64_t &getCreateTime() const { return m_createTime; }

        // 设置日志级别
        void setLevel(LogLevel::Level level) { m_level = level; }

        // 获取日志级别
        LogLevel::Level getLevel() const { return m_level; }

        // 添加日志目标
        void addAppender(LogAppender::ptr appender);

        // 删除日志目标
        void delAppender(LogAppender::ptr appender);

        // 清空日志目标
        void clearAppenders();

        // 写日志
        void log(LogEvent::ptr event);
        
        // 将日志器的配置转成YAML String
        std::string toYamlString();

    private:
        // 日志器名称
        std::string m_name;
        // 日志级别
        LogLevel::Level m_level;
        // Mutex
        MutexType m_mutex;
        // 日志目标集合
        std::list<LogAppender::ptr> m_appenders;
        // 创建时间（毫秒）
        uint64_t m_createTime;
    };

    // 日志事件包装器，方便宏定义，内部包含日志事件和日志器
    class LogEventWrap
    {
    public:
        LogEventWrap(Logger::ptr logger, LogEvent::ptr event);

        ~LogEventWrap(); // 日志事件在析构时由日志器进行输出

        // 获取日志事件
        LogEvent::ptr getLogEvent() const { return m_event; }

    private:
        Logger::ptr m_logger;
        LogEvent::ptr m_event;
    };

    // 日志器管理类
    class LoggerManager
    {
    public:
        typedef Spinlock MutexType;
        LoggerManager();
        Logger::ptr getLogger(const std::string &name); // 获取指定名称的日志器
        void init(); // 初始化，主要是结合配置模块实现日志模块初始化
        Logger::ptr getRoot() { return m_root; } //获取root日志器，等效于getLogger("root")
        std::string toYamlString(); //将所有的日志器配置转成YAML String

    private:
        MutexType m_mutex;
        std::map<std::string, Logger::ptr> m_loggers;// 日志器容器：日志器集合
        Logger::ptr m_root; // root日志器 
    };

    // 日志器管理类单例
    // LoggerManager类采用单例模式实现
    // LoggerMgr是LoggerManager类的单例对象，通过这个单例对象可以全局访问 LoggerManager 的功能，确保在整个程序中只有一个 LoggerManager 实例存在
    typedef sy::Singleton<LoggerManager> LoggerMgr;
}

#endif