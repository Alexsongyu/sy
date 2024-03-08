#include "sy/config.h"
#include "sy/env.h"
#include "sy/util.h"
#include <sys/types.h>                                                                                                                                                    o  
#include <sys/stat.h>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
#include <unistd.h>

namespace sy
{

    static sy::Logger::ptr g_logger = SY_LOG_NAME("system");

    // 根据参数名称查找配置参数，返回对应配置参数的值
    ConfigVarBase::ptr Config::LookupBase(const std::string &name)
    {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        return it == GetDatas().end() ? nullptr : it->second; 
    }

    // 递归遍历YAML格式的配置文件中的所有成员，将每个节点ABC的名称和值存在列表中
    // YAML格式：
                // A:
                //  B: 10
                //  C: str
    // 列表的键值对格式：
                //("A.B", 10) 和 ("A.C", "str")
    static void ListAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        // prefix前缀字符不合法则记录错误信息并返回
        if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos)
        {
            SY_LOG_ERROR(g_logger) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        // 否则将当前节点node的名称和值存储在输出列表output中
        output.push_back(std::make_pair(prefix, node));
        // 如果当前节点是一个映射，则递归遍历每个子节点，并根据前缀构建完整的节点名称
        if (node.IsMap())
        {
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                // 若前缀为空,说明为顶层，prefix为key的值；否则为子层，prefix为父层加上当前层
                ListAllMember(prefix.empty() ? it->first.Scalar()
                                             : prefix + "." + it->first.Scalar(),
                              it->second, output);
            }
        }
    }
    // 从 YAML 文件加载配置
    void Config::LoadFromYaml(const YAML::Node &root)
    {
        // 将YAML::Node对象root中的所有节点信息保存到all_nodes中
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);

         // 遍历all_nodes列表，查找对应的配置参数并根据节点类型进行转换
        for (auto &i : all_nodes)
        {
            std::string key = i.first;
            if (key.empty()) continue;
            // 无视大小写
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            // 查找名为key的配置参数
            ConfigVarBase::ptr var = LookupBase(key);
            // 若找到
            if (var)
            {
                // 若为纯量Scalar，则调用fromString(会调用setValue设值)
                if (i.second.IsScalar())
                {
                    var->fromString(i.second.Scalar());
                }
                // 否则为数组，将其转换为字符串
                else
                {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

    static std::map<std::string, uint64_t> s_file2modifytime;
    static sy::Mutex s_mutex;

    // 加载指定路径下的配置文件（.yml 文件）
    void Config::LoadFromConfDir(const std::string &path, bool force)
    {
        std::string absoulte_path = sy::EnvMgr::GetInstance()->getAbsolutePath(path);
        std::vector<std::string> files;
        FSUtil::ListAllFile(files, absoulte_path, ".yml");

        // 遍历文件列表，检查文件的修改时间，如果文件未修改且不强制加载，则跳过
        for (auto &i : files)
        {
            {
                struct stat st;
                lstat(i.c_str(), &st);
                sy::Mutex::Lock lock(s_mutex);
                if (!force && s_file2modifytime[i] == (uint64_t)st.st_mtime)
                {
                    continue;
                }
                s_file2modifytime[i] = st.st_mtime;
            }
            try
            {
                //  通过 YAML::LoadFile 加载 YAML 文件，然后调用 LoadFromYaml 加载配置信息
                YAML::Node root = YAML::LoadFile(i);
                LoadFromYaml(root);
                SY_LOG_INFO(g_logger) << "LoadConfFile file="
                                      << i << " ok";
            }
            catch (...)
            {
                SY_LOG_ERROR(g_logger) << "LoadConfFile file="
                                       << i << " failed";
            }
        }
    }

    // 遍历配置模块中的所有配置项，并对每个配置项执行传入的回调函数
    void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
    {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap &m = GetDatas();
        for (auto it = m.begin();
             it != m.end(); ++it)
        {
            cb(it->second);
        }
    }

}
