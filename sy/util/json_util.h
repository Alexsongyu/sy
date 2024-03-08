#ifndef __SY_UTIL_JSON_UTIL_H__
#define __SY_UTIL_JSON_UTIL_H__

#include <string>
#include <iostream>

// 使用 extern "C" 包围 C 语言的头文件：告诉编译器这是C语言的头文件
#ifdef __cplusplus
extern "C" {
#endif

#include <json/json.h>

#ifdef __cplusplus
}
#endif

namespace sy {

class JsonUtil {
public:
    static bool NeedEscape(const std::string& v);
    static std::string Escape(const std::string& v);
    static std::string GetString(const Json::Value& json
                          ,const std::string& name
                          ,const std::string& default_value = "");
    static double GetDouble(const Json::Value& json
                     ,const std::string& name
                     ,double default_value = 0);
    static int32_t GetInt32(const Json::Value& json
                     ,const std::string& name
                     ,int32_t default_value = 0);
    static uint32_t GetUint32(const Json::Value& json
                       ,const std::string& name
                       ,uint32_t default_value = 0);
    static int64_t GetInt64(const Json::Value& json
                     ,const std::string& name
                     ,int64_t default_value = 0);
    static uint64_t GetUint64(const Json::Value& json
                       ,const std::string& name
                       ,uint64_t default_value = 0);
    static bool FromString(Json::Value& json, const std::string& v);
    static std::string ToString(const Json::Value& json);
};

}

#endif
