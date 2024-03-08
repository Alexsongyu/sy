/**
 * @file noncopyable.h
 * @brief 不可拷贝对象封装
 * @author sy.yin
 * @email 564628276@qq.com
 * @date 2019-05-31
 * @copyright Copyright (c) 2019年 sy.yin All rights reserved (www.sy.top)
 */
#ifndef __SY_NONCOPYABLE_H__
#define __SY_NONCOPYABLE_H__

namespace sy {

/**
 * @brief 对象无法拷贝,赋值
 */
class Noncopyable {
public:
    /**
     * @brief 默认构造函数
     */
    Noncopyable() = default;

    /**
     * @brief 默认析构函数
     */
    ~Noncopyable() = default;

    /**
     * @brief 拷贝构造函数(禁用)
     */
    Noncopyable(const Noncopyable&) = delete;

    /**
     * @brief 赋值函数(禁用)
     */
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif
