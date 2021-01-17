/**
 * @file sync_queue.hpp
 * @author wotsen (astralrovers@outlook.com)
 * @brief
 * @date 2021-01-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include <stddef.h>

namespace Asty {

/**
 * @brief 同步队列抽象接口
 *
 * @tparam T 队列对象类型
 */
template <typename T>
class ISyncQueue {
public:
    virtual ~ISyncQueue() {}

    // 入队
    virtual bool push(T&& obj) = 0;
    // 出队
    virtual bool pop(T& t) = 0;

    // 队列数量
    virtual size_t count(void) = 0;
    // 队列空判
    virtual bool empty(void) = 0;
    // 队列满判
    virtual bool full(void) = 0;
    // 退出队列
    virtual void stop(void) = 0;
};

}
