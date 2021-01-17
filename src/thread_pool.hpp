/**
 * @file thread_pool.hpp
 * @author wotsen (astralrovers@outlook.com)
 * @brief
 * @date 2021-01-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <functional>

/**
 * @brief 任务接口类型
 * 
 */
typedef std::function<void(void)> thread_task_t;

/**
 * @brief 线程池基类
 *
 */
class IThreadPool {
public:
    virtual ~IThreadPool() {}

    // 启动线程池
    virtual void run() = 0;
    // 退出线程池
    virtual void stop() = 0;

    // 添加任务
    virtual void add_task(const thread_task_t &task) = 0;
};
