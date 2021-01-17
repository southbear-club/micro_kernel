/**
 * @file micro_sync_task_queue.hpp
 * @author wotsen (astralrovers@outlook.com)
 * @brief
 * @date 2021-01-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include "sync_queue.hpp"

namespace Asty {

/**
 * @brief 微内核任务队列
 * 
 * @tparam T 队列类型
 */
template <typename T>
class MicroSyncTaskQueue : public ISyncQueue<T> {
public:
    MicroSyncTaskQueue(int size) : max_size_(size) {}
    virtual ~MicroSyncTaskQueue() { stop(); }

    // 入队
    virtual bool push(T&& obj) override {
        std::unique_lock<std::mutex> lck(mutex_);
        // 等待队列非满才能入队
        not_full_.wait(lck, [this] { return stop_ || (count() < max_size_); });

        if (stop_) {
            return false;
        }

        queue_.push_back(std::forward<T>(obj));
        not_empty_.notify_one();

        return true;
    }

    // 出队
    virtual bool pop(T& t) override {
        std::unique_lock<std::mutex> lck(mutex_);
        // 等待队列非空才能出队
        not_empty_.wait(lck, [this] { return stop_ || !queue_.empty(); });

        if (stop_) {
            return false;
        }

        t = queue_.front();
        queue_.pop_front();
        not_full_.notify_one();

        return true;
    }

    // 队列内容数
    virtual int count(void) override { return queue_.size(); }

    virtual bool empty(void) override { return queue_.empty(); }

    virtual bool full(void) override { return queue_.size() >= max_size_; }

    // 停止队列
    virtual void stop(void) override {
        {
            std::unique_lock<std::mutex> lck(mutex_);
            stop_ = true;
        }

        not_full_.notify_all();
        not_empty_.notify_all();
    }

private:
    std::list<T> queue_;                 ///< 任务队列
    std::mutex mutex_;                   ///< 队列锁
    std::condition_variable not_empty_;  ///< 非空条件变量
    std::condition_variable not_full_;   ///< 非满条件变量
    int max_size_;                       ///< 任务队列限制
    bool stop_;                          ///< 退出条件
};

}
