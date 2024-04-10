#ifndef __SY_TIMER_H__
#define __SY_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include "thread.h"

namespace sy {

class TimerManager;
// 定时器
class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    // 定时器的智能指针类型：lhs和rhs
    typedef std::shared_ptr<Timer> ptr;

    // 取消定时器
    bool cancel();

    // 刷新设置定时器的执行时间
    bool refresh();

    // 重置定时器时间
    // from_now 是否从当前时间开始计算
    bool reset(uint64_t ms, bool from_now);
private:
    // 构造函数私有：只能通过TimerManager类来创建Timer对象
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);
    Timer(uint64_t next);
private:
    // 是否循环定时器
    bool m_recurring = false;
    // 执行周期:定时器执行间隔时间(毫秒)
    uint64_t m_ms = 0;
    // 精确的执行时间（毫秒）
    uint64_t m_next = 0;
    // 回调函数
    std::function<void()> m_cb;
    // 定时器的管理器
    TimerManager* m_manager = nullptr;
private:
    // 定时器比较仿函数：比较定时器的智能指针的大小，按照执行时间的先后对定时器排序
    // 这样方便定时器的管理器来确定下一个要执行的定时器是谁
    struct Comparator {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

// 定时器管理器：管理所有的Timer对象
class TimerManager {
friend class Timer;
public:
    // 读写锁类型
    typedef RWMutex RWMutexType;

    // 构造函数
    TimerManager();

    // 析构函数
    virtual ~TimerManager();

    // 添加定时器
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    // 添加条件定时器
    // weak_cond 条件
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, bool recurring = false
                        ,std::weak_ptr<void> weak_cond);

    // 到最近一个定时器执行的时间间隔(毫秒)
    uint64_t getNextTimer();

    // 获取需要执行的定时器的回调函数列表
    // cbs 回调函数数组
    void listExpiredCb(std::vector<std::function<void()> >& cbs);

    // 是否有定时器
    bool hasTimer();
protected:
    // 当有新的定时器插入到定时器的首部，执行该函数
    // 通知继承了 TimerManager 的类（如 IOManager）立即更新当前的 epoll_wait 超时时间
    // 这样可以确保在新定时器插入时，及时更新事件循环的超时时间，以便及时处理新的定时器事件
    virtual void onTimerInsertedAtFront() = 0;

    // 将定时器添加到管理器中
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
private:
    // 检测服务器时间是否被调后了
    bool detectClockRollover(uint64_t now_ms);
private:
    // Mutex
    RWMutexType m_mutex;
    // 定时器集合：由于set会自动对定时器对象按照执行时间的先后顺序进行排序，因此管理器可以很快确定下一个要执行的定时器
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    // 是否触发onTimerInsertedAtFront
    bool m_tickled = false;
    // 上次执行时间
    uint64_t m_previouseTime = 0;
};

}

#endif
