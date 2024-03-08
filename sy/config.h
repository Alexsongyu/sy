// 头文件保护宏：如果未定义该宏则执行下面代码
// 确保这个文件的头文件只被包含一次
#ifndef __SY_CONFIG_H__ 
#define __SY_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp> // boost库中的类型转换头文件
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "thread.h"
#include "log.h"
#include "util.h"

// 定义类和模板
namespace sy
{

    // 配置变量的基类
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        // 构造函数初始化：配置参数的名称和描述
        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name), m_description(description)
        {
            // 将 m_name 的字符转化为小写形式，从而可以不区分大小写
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase() {}

        const std::string &getName() const { return m_name; }
        const std::string &getDescription() const { return m_description; }

        // 使用纯虚函数进行配置参数的转换和解析：基类提供统一的接口，子类根据自身特性来调用，实现多态
        virtual std::string toString() = 0; // 转成字符串
        virtual bool fromString(const std::string &val) = 0; // 解析：从字符串初始化值
        virtual std::string getTypeName() const = 0; // 返回配置参数值的类型名称

    protected:
        std::string m_name;
        std::string m_description;
    };

    // 类型转换模板类(F 源类型, T 目标类型)
    // 这里的LexicalCast类是一个仿函数，它可将传入的F类型的参数v进行转换，并返回转换后的参数的目标类型T
    template <class F, class T>
    class LexicalCast
    {
    public:
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    // 以下是 std::string（YAML格式的字符串）和 各个目标类型 互相转换的模板类

    // YAML String 转换成 std::vector<T>："[1, 2, 3]" ——> [1, 2, 3]
    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            // 首先将输入的 YAML 字符串 v 转换为 YAML 节点 node
            // 然后创建一个空的 std::vector<T> 类型的变量 vec 用于存储转换后的结果
            // 遍历 YAML 节点 node 中的每个元素，将每个元素转换为字符串并调用 LexicalCast<std::string, T>() 进行类型转换
            // 最后将转换后的值添加到 vec 中，最后返回转换后的 std::vector<T> 类型的结果
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // std::vector<T> 转换成 YAML String
    // [1, 2, 3] ——> - 1
    //               - 2
    //               - 3    
    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // YAML String 转换成 std::list<T>
    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // std::list<T> 转换成 YAML String
    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // YAML String 转换成 std::set<T>
    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // std::set<T> 转换成 YAML String
    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // YAML String 转换成 std::unordered_set<T>
    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // std::unordered_set<T> 转换成 YAML String
    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // YAML String 转换成 std::map<std::string, T>
    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    // std::map<std::string, T> 转换成 YAML String
    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            YAML::Node node(YAML::NodeType::Map);
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // YAML String 转换成 std::unordered_map<std::string, T>
    template <class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    // std::unordered_map<std::string, T> 转换成 YAML String
    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            YAML::Node node(YAML::NodeType::Map);
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 配置参数模板子类：保存对应类型的参数值
    // FromStr 配置参数从std::string转换成T类型
    // ToStr 配置参数从T类型转换成std::string
    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        typedef RWMutex RWMutexType; //读写锁类型，允许多个线程同时读取参数值，确保只有一个线程可以修改参数值，避免并发写入导致数据不一致
        typedef std::shared_ptr<ConfigVar> ptr; // 智能指针类型，管理 ConfigVar 类的实例，自动释放对象从而避免内存泄漏
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb; // 回调函数类型，处理参数值变化时的回调函数

        // 构造函数：初始化 基类和自己类的 配置参数
        ConfigVar(const std::string &name, const std::string &description = "", const T &default_value)
            : ConfigVarBase(name, description), m_val(default_value){}

        // 成员函数
        // 将参数值转换成 YAML格式的字符串
        std::string toString() override //override表示toString()覆盖了基类相同名称的虚函数
        {
            try // 尝试对参数值进行转换，转换成功则返回转换后的 YAML格式的字符串
            {
                RWMutexType::ReadLock lock(m_mutex);
                return ToStr()(m_val); 
            }
            catch (std::exception &e) // 当转换失败抛出异常，记录错误日志的异常信息、转换类型和参数名称
            {
                SY_LOG_ERROR(SY_LOG_ROOT()) << "ConfigVar::toString exception "
                                            << e.what() << " convert: " << TypeToName<T>() << " to string"
                                            << " name=" << m_name;
            }
            return "";// 转换失败返回空字符串
        }

        // 从YAML字符串转成参数的值
        bool fromString(const std::string &val) override
        {
            try
            {
                setValue(FromStr()(val));
            }
            catch (std::exception &e) // 当转换失败抛出异常
            {
                SY_LOG_ERROR(SY_LOG_ROOT()) << "ConfigVar::fromString exception "
                                            << e.what() << " convert: string to " << TypeToName<T>()
                                            << " name=" << m_name
                                            << " - " << val;
            }
            return false;
        }

        // 获取当前参数的值
        const T getValue()
        {
            RWMutexType::ReadLock lock(m_mutex);
            return m_val;
        }

        // 设置当前参数的值
        // 如果原来参数的值和当前参数的值 相同则直接返回；不同则通知对应的注册回调函数
        void setValue(const T &v)
        {
            {
                RWMutexType::ReadLock lock(m_mutex);
                if (v == m_val)
                {
                    return;
                }
                for (auto &i : m_cbs)
                {
                    i.second(m_val, v);
                }
            }
            RWMutexType::WriteLock lock(m_mutex);
            m_val = v;
        }

        // 返回参数值的类型名称(typeinfo)
        std::string getTypeName() const override { return TypeToName<T>(); }

        // 添加变化回调函数：返回该回调函数的唯一id,用于删除回调函数
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        // 删除回调函数：key 回调函数的唯一id，删除指定id对应的回调函数
        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }

        // 获取回调函数：如果存在 则返回 指定id对应的回调函数,否则返回nullptr
        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        // 清除所有的回调函数
        void clearListener()
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }

    private:
        mutable RWMutexType m_mutex; // 可变的读写锁，用于保护参数值的读写操作
        T m_val; 
        std::map<uint64_t, on_change_cb> m_cbs; // 存储变更回调函数的映射, uint64_t表示使用 唯一的 id作为key
    };

    // ConfigVar的管理类：提供便捷的方法创建/访问ConfigVar类的实例
    class Config
    {
    public:
        typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        typedef RWMutex RWMutexType;

        // 静态成员函数 Lookup：创建对应参数名的配置参数
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name,
                                                 const T &default_value, const std::string &description = "")
        {
            RWMutexType::WriteLock lock(GetMutex()); // 写锁
            auto it = GetDatas().find(name); // 查找名为name的配置参数
            // 找到了
            if (it != GetDatas().end())
            {
                // 将其转换为 ConfigVar<T> 类型，并返回该实例
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                // 若转换成功，即参数tmp存在，则返回参数名
                if (tmp) 
                {
                    SY_LOG_INFO(SY_LOG_ROOT()) << "Lookup name=" << name << " exists";
                    return tmp;
                }
                else 
                {
                    SY_LOG_ERROR(SY_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                                                << TypeToName<T>() << " real_type=" << it->second->getTypeName()
                                                << " " << it->second->toString();
                    return nullptr;
                }
            }

            // 用于在当前字符串中查找第一个不属于指定字符集合的字符，并返回该字符位置的索引。如果没有找到任何字符，则返回 std::string::npos。
            // name不全在 "abcdefghigklmnopqrstuvwxyz._012345678" 中 
            // name中有非法字符，抛出异常 std::invalid_argument
            if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos)
            {
                SY_LOG_ERROR(SY_LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            // 若没有找到，则创建一个新的ConfigVar<T> 实例（名为 name 的配置参数 v），并其添加到配置项映射中，最后返回该实例
            // typename：用于告诉编译器 ConfigVar<T>::ptr 是一个类型而不是成员变量
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        // 查找配置参数，找到了则返回配置参数名为name的配置参数
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            RWMutexType::ReadLock lock(GetMutex());
            auto it = GetDatas().find(name);
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            // 利用 dynamic_pointer_cast 将 基类ConfigVarBase的智能指针 it->second 强制转换为子类 ConfigVar<T>类型的智能指针，并返回转换后的结果
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        // 使用YAML::Node初始化配置模块
        static void LoadFromYaml(const YAML::Node &root);

        // 加载path文件夹里面的配置文件
        static void LoadFromConfDir(const std::string &path, bool force = false);

        // 查找配置参数,返回配置参数的基类
        static ConfigVarBase::ptr LookupBase(const std::string &name);

        // 遍历配置模块里面所有配置项，接受一个 cb配置项回调函数 作为参数
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        // 使用静态方法返回参数，保证初始化顺序：函数内的static对象会在 “该函数被调用期间 首次遇上该对象之定义式” 时被初始化

        // 返回配置项映射，确保静态对象在首次调用时被初始化
        static ConfigVarMap &GetDatas()
        {
            static ConfigVarMap s_datas;
            return s_datas;
        }

        // 返回配置项的读写锁，确保静态对象在首次调用时被初始化
        static RWMutexType &GetMutex()
        {
            static RWMutexType s_mutex;
            return s_mutex;
        }
    };

}

#endif
