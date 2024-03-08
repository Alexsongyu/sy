#include "sy/config.h"
#include "sy/log.h"
#include <yaml-cpp/yaml.h>
#include "sy/env.h"
#include <iostream>

#if 1
// 通过 sy::Config::Lookup 方法创建和初始化各种类型的配置变量
// 例如：设置默认端口号，名称：system.port，值：8080，描述：system port
sy::ConfigVar<int>::ptr g_int_value_config =
    sy::Config::Lookup("system.port", (int)8080, "system port");

sy::ConfigVar<float>::ptr g_int_valuex_config =
    sy::Config::Lookup("system.port", (float)8080, "system port");

sy::ConfigVar<float>::ptr g_float_value_config =
    sy::Config::Lookup("system.value", (float)10.2f, "system value");

sy::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config =
    sy::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int vec");

sy::ConfigVar<std::list<int> >::ptr g_int_list_value_config =
    sy::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int list");

sy::ConfigVar<std::set<int> >::ptr g_int_set_value_config =
    sy::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int set");

sy::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config =
    sy::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int uset");

sy::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config =
    sy::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",2}}, "system str int map");

sy::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config =
    sy::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",2}}, "system str int map");

// 测试如何加载和遍历YAML
void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) {
        SY_LOG_INFO(SY_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        SY_LOG_INFO(SY_LOG_ROOT()) << std::string(level * 4, ' ')
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            SY_LOG_INFO(SY_LOG_ROOT()) << std::string(level * 4, ' ')
                    << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            SY_LOG_INFO(SY_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

// 测试 YAML 文件的加载和解析功能
// 加载 YAML 文件并使用 print_yaml() 递归遍历 YAML 节点，打印出节点信息
void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/sy/workspace/sy/bin/conf/log.yml");
    //print_yaml(root, 0);
    //SY_LOG_INFO(SY_LOG_ROOT()) << root.Scalar();

    SY_LOG_INFO(SY_LOG_ROOT()) << root["test"].IsDefined();
    SY_LOG_INFO(SY_LOG_ROOT()) << root["logs"].IsDefined();
    SY_LOG_INFO(SY_LOG_ROOT()) << root;
}

// 测试配置变量的加载和变更
// 在加载 YAML 配置文件前后，通过宏 XX 和 XX_M 打印不同类型的配置变量的值
// 通过 sy::Config::LoadFromYaml 方法加载 YAML 配置文件来更新配置变量的值
void test_config() {
    SY_LOG_INFO(SY_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SY_LOG_INFO(SY_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            SY_LOG_INFO(SY_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        SY_LOG_INFO(SY_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            SY_LOG_INFO(SY_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        SY_LOG_INFO(SY_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }


    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/sy/workspace/sy/bin/conf/test.yml");
    sy::Config::LoadFromYaml(root);

    SY_LOG_INFO(SY_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SY_LOG_INFO(SY_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}

#endif

class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace sy {

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}

sy::ConfigVar<Person>::ptr g_person =
    sy::Config::Lookup("class.person", Person(), "system person");

sy::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    sy::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

sy::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    sy::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");

// 测试自定义类型的配置变量：自定义的Person类和为其实现的 LexicalCast 模板特化
// 创建和管理自定义类型的配置变量，监听配置变量的变更事件
void test_class() {
    SY_LOG_INFO(SY_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            SY_LOG_INFO(SY_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        SY_LOG_INFO(SY_LOG_ROOT()) <<  prefix << ": size=" << m.size(); \
    }

    g_person->addListener([](const Person& old_value, const Person& new_value){
        SY_LOG_INFO(SY_LOG_ROOT()) << "old_value=" << old_value.toString()
                << " new_value=" << new_value.toString();
    });

    XX_PM(g_person_map, "class.map before");
    SY_LOG_INFO(SY_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/sy/workspace/sy/bin/conf/test.yml");
    sy::Config::LoadFromYaml(root);

    SY_LOG_INFO(SY_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    SY_LOG_INFO(SY_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

// 加载日志配置文件，展示如何配置和使用日志系统
void test_log() {
    static sy::Logger::ptr system_log = SY_LOG_NAME("system");
    SY_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << sy::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/sy/workspace/sy/bin/conf/log.yml");
    sy::Config::LoadFromYaml(root);
    std::cout << "=============" << std::endl;
    std::cout << sy::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    SY_LOG_INFO(system_log) << "hello system" << std::endl;

    system_log->setFormatter("%d - %m%n");
    SY_LOG_INFO(system_log) << "hello system" << std::endl;
}

// 展示如何加载指定目录下的所有配置文件
void test_loadconf() {
    sy::Config::LoadFromConfDir("conf");
}

int main(int argc, char** argv) {
    //test_yaml();
    //test_config();
    //test_class();
    //test_log();
    sy::EnvMgr::GetInstance()->init(argc, argv);
    test_loadconf();
    std::cout << " ==== " << std::endl;
    sleep(10);
    test_loadconf();
    return 0;
    sy::Config::Visit([](sy::ConfigVarBase::ptr var) {
        SY_LOG_INFO(SY_LOG_ROOT()) << "name=" << var->getName()
                    << " description=" << var->getDescription()
                    << " typename=" << var->getTypeName()
                    << " value=" << var->toString();
    });

    return 0;
}
