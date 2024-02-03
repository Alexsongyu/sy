// 定义宏，防止头文件被多次引用
#ifndef _SY_LOG_H // if not defined，则继续执行
#define _SY_LOG_H

#include <string>
#include <stdint.h>
#include <memory> // 指针
#include <list>
#include <stringstream>
#include <fstring>

// 定义类，防止别人和这里面的 类、变量、函数重名
namespace sy{

// 日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent();
private:
    //文件名：不可修改。变量定义加 m_ 区分局部变量
    const char* m_file = nullptr;
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
    std::string m_content;
};

// 日志级别
class LogLevel{
public:
    enum Level{    // 枚举类型
        DEBUG = 1;
        INFO = 2;
        WARN = 3;
        ERROR = 4;
        FATAL = 5;
    };
};

// 定义日志输出格式
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern); // 实例化 pattern

    std::string format(LogEvent::ptr event); //传入 event 返回 string
private:
    // 子类
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem()
        virtual void format(std::ostream& os, LogEvent::ptr event) = 0; //传入 event 返回 ostream
    };

    void init(); // pattern的解析
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items; //
};

// 日志输出地点
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender(){}// 虚析构：确保正确释放子类对象资源

    virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0; //将虚函数声明为纯虚函数：基类没有默认实现，需要子类实现这个纯虚函数才能被实例化
    
    LogFormatter::ptr getFormatter() const {return m_formatter;}
    void setFormatter(LogFormatter::ptr val) {m_formatter = val;}
protected:// 后面的子类要用到
    LogLevel::level m_level;// LogLevel 类中的 level 枚举类型
    LogFormatter::ptr m_formatter;
};

// 日志器
class Logger{
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name = "root");
    void Log(Level level, const LogEvent& event );// 成员函数，用于记录日志的 level 和 event

    // 定义：输出不同级别的日志；cpp中实现这些成员函数
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    LogLevel::Level getLevel() const {return m_level;}
    void setLevel(LogLevel::Level val) {m_level = val;} 
private:
    std::string m_name; // 日志名称
    LogLevel::Level m_level;
    std::list<LogAppender::ptr> m_appenders; // Appender集合
};

// 输出到控制台的 Appender
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<stdoutLogAppender> ptr;
    void log(LogLevel::Level level, LogEvent::ptr event) override; //重载
private:
};

// 输出到文件的 Appender
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename); // 文件名
    void log(LogLevel::Level level, LogEvent::ptr event) override;
    bool reopen(); //文件重新打开成功，返回true
private:
    std::string m_name;
    std::ofstream m_filestream;
};

}

#endif