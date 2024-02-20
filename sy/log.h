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
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"

// 记录不同 level 的日志
// 使用流式方式将 指定level日志 写入到logger
#define SY_LOG_LEVEL(logger, level)                                                                         \
    if (logger->getLevel() <= level)                                                                        \
    sy::LogEventWrap(sy::LogEvent::ptr(new sy::LogEvent(logger, level,                                      \
                                                        __FILE__, __LINE__, 0, sy::GetThreadId(),           \
                                                        sy::GetFiberId(), time(0), sy::Thread::GetName()))) \
        .getSS()

#define SY_LOG_DEBUG(logger) SY_LOG_LEVEL(logger, sy::LogLevel::DEBUG)
#define SY_LOG_INFO(logger) SY_LOG_LEVEL(logger, sy::LogLevel::INFO)
#define SY_LOG_WARN(logger) SY_LOG_LEVEL(logger, sy::LogLevel::WARN)
#define SY_LOG_ERROR(logger) SY_LOG_LEVEL(logger, sy::LogLevel::ERROR)
#define SY_LOG_FATAL(logger) SY_LOG_LEVEL(logger, sy::LogLevel::FATAL)

// 使用格式化方式将 指定level的格式化日志 写入到logger
#define SY_LOG_FMT_LEVEL(logger, level, fmt, ...)                                                           \
    if (logger->getLevel() <= level)                                                                        \
    sy::LogEventWrap(sy::LogEvent::ptr(new sy::LogEvent(logger, level,                                      \
                                                        __FILE__, __LINE__, 0, sy::GetThreadId(),           \
                                                        sy::GetFiberId(), time(0), sy::Thread::GetName()))) \
        .getEvent()                                                                                         \
        ->format(fmt, __VA_ARGS__)

#define SY_LOG_FMT_DEBUG(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SY_LOG_FMT_INFO(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::INFO, fmt, __VA_ARGS__)
#define SY_LOG_FMT_WARN(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::WARN, fmt, __VA_ARGS__)
#define SY_LOG_FMT_ERROR(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SY_LOG_FMT_FATAL(logger, fmt, ...) SY_LOG_FMT_LEVEL(logger, sy::LogLevel::FATAL, fmt, __VA_ARGS__)

// 获取主日志器
#define SY_LOG_ROOT() sy::LoggerMgr::GetInstance()->getRoot()

// 获取name的日志器
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
            UNKNOW = 0,
            DEBUG = 1;
            INFO = 2;
            WARN = 3;
            ERROR = 4;
            FATAL = 5;
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
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string &thread_name);

        // 返回文件名
        const char *getFile() const { return m_file; }
        // 返回行号
        int32_t getLine() const { return m_line; }
        // 返回程序启动到现在的毫秒数
        uint32_t getElapse() const { return m_elapse; }
        // 返回线程ID
        uint32_t getThreadId() const { return m_threadId; }
        // 返回协程ID
        uint32_t getFiberId() const { return m_fiberId; }
        // 返回时间
        uint64_t getTime() const { return m_time; }
        // 返回线程名称
        const std::string &getThreadName() const { return m_threadName; }
        // 返回日志内容
        std::string getContent() const { return m_ss.str(); }
        // 返回日志器
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        // 返回日志级别
        LogLevel::Level getLevel() const { return m_level; }
        // 返回日志内容字符串流
        std::stringstream &getSS() { return m_ss; }
        // 格式化写入日志内容
        void format(const char *fmt, ...);
        // 格式化写入日志内容
        void format(const char *fmt, va_list al);

    private:
        // 文件名：不可修改。变量定义加 m_ 区分局部变量
        const char *m_file = nullptr;
        // 行号
        int32_t m_line = 0;
        // 程序启动到现在的毫秒数
        uint32_t m_elapse = 0;
        // 线程 ID
        uint32_t m_threadID = 0;
        // 协程 ID
        uint32_t m_fiberID = 0;
        // 时间戳
        uint64_t m_time = 0;
        // 线程名称
        std::string m_threadName;
        // 日志内容
        std::stringstream m_ss;
        // 日志器
        std::shared_ptr<Logger> m_logger;
        // 日志级别
        LogLevel::Level m_level;
    };

    // 日志事件包装器
    class LogEventWrap
    {
    public:
        // 构造函数 e 日志事件
        LogEventWrap(LogEvent::ptr e);

        ~LogEventWrap();

        // 获取日志事件
        LogEvent::ptr getEvent() const { return m_event; }

        // 获取日志内容流
        std::stringstream &getSS();

    private:
        // 日志事件
        LogEvent::ptr m_event;
    };

    // 定义日志输出格式
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string &pattern); // 实例化 日志模板pattern

        // 返回格式化的日志：传入 event 返回 string
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
        std::ostream &format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    private:
        // 子类：日志内容项格式化
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem()
                // 格式化日志到流：os是日志输出流，传入 event 返回 ostream
                virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        void init();                                               // pattern的解析
        bool isError() const { return m_error; }                   // 判断是否出错
        const std::string getPattern() const { return m_pattern; } // 返回pattern
    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items; // 日志格式解析后的格式
        bool m_error = false;
    };

    // 日志输出地点
    class LogAppender
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Spinlock MutexType;

        virtual ~LogAppender() {} // 虚析构：确保正确释放子类对象资源
        // 写入日志
        // 将虚函数声明为纯虚函数：基类没有默认实现，需要子类实现这个纯虚函数才能被实例化
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        // 将日志输出目标的配置转成YAML String
        virtual std::string toYamlString() = 0;
        void setFormatter(LogFormatter::ptr val); // 更改日志格式器
        LogFormatter::ptr getFormatter();         // 获取日志格式器
        void setLevel(LogLevel::Level val) { m_level = val; }
        LogLevel::Level getLevel() const { return m_level; }

    protected: // 后面的子类要用到
        // 日志级别
        LogLevel::Level m_level = LogLevel::DEBUG;
        // 是否有自己的日志格式器
        bool m_hasFormatter = false;
        // Mutex
        MutexType m_mutex;
        // 日志格式器
        LogFormatter::ptr m_formatter;
    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManager;

    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Spinlock MutexType;

        // 构造函数 name 日志器名称
        Logger(const std::string &name = "root");

        // 写日志
        void log(LogLevel::Level level, LogEvent::ptr event);
        // 写debug级别日志
        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);
        // 添加 日志目标appender
        void addAppender(LogAppender::ptr appender);
        // 删除日志目标
        void delAppender(LogAppender::ptr appender);
        // 清空日志目标
        void clearAppenders();

        // 返回日志级别
        LogLevel::Level getLevel() const { return m_level; }

        // 设置日志级别
        void setLevel(LogLevel::Level val) { m_level = val; }

        // 返回日志名称
        const std::string &getName() const { return m_name; }

        // 设置日志格式器
        void setFormatter(LogFormatter::ptr val);

        // 设置日志格式模板
        void setFormatter(const std::string &val);
        // 获取日志格式器
        LogFormatter::ptr getFormatter();

        // 将日志器的配置转成YAML String
        std::string toYamlString();

    private:
        // 日志名称
        std::string m_name;
        // 日志级别
        LogLevel::Level m_level;
        // Mutex
        MutexType m_mutex;
        // 日志目标集合
        std::list<LogAppender::ptr> m_appenders;
        // 日志格式器
        LogFormatter::ptr m_formatter;
        // 主日志器
        Logger::ptr m_root;
    };

    // 输出到控制台的 Appender
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<stdoutLogAppender> ptr;
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override; // 重载
        std::string toYamlString() override;
    };

    // 输出到文件的 Appender
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename); // 文件名
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;

        bool reopen(); // 日志文件重新打开成功，返回true
    private:
        std::string m_filename;     // 文件路径
        std::ofstream m_filestream; // 文件流
        uint64_t m_lastTime = 0;    // 上次重新打开的时间
    };

    // 日志器管理类
    class LoggerManager
    {
    public:
        typedef Spinlock MutexType;
        LoggerManager();
        Logger::ptr getLogger(const std::string &name);
        void init();
        Logger::ptr getRoot() const { return m_root; }
        std::string toYamlString();

    private:
        MutexType m_mutex;
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    // 日志器管理类单例模式
    typedef sylar::Singleton<LoggerManager> LoggerMgr;
}

#endif