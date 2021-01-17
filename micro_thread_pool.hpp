/**
 * @file micro_thread_pool.hpp
 * @author wotsen (astralrovers@outlook.com)
 * @brief 微内核线程池
 * @date 2021-01-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include <atomic>
#include <future>
#include <memory>
#include "micro_sync_task_queue.hpp"
#include "thread_pool.hpp"

namespace Asty {

/**
 * @brief 微内核线程池
 * 
 */
class MicroKernelThreadPool : public IThreadPool {
public:
    MicroKernelThreadPool(int task_limit = 100,
                          int thread_cnt = std::thread::hardware_concurrency())
        : queue_(task_limit), running_(false) {
        running_ = true;
        for (int i = 0; i < thread_cnt; i++) {
            threads_.push_back(
                std::make_shared<std::thread>([this] { run(); }));
        }
    }

    virtual ~MicroKernelThreadPool() { stop(); }

    virtual void run() override {
        while (running_) {
            thread_task_t t = nullptr;
            auto ret = queue_.pop(t);

            if (!ret || !t || !running_) {
                return;
            }

            t();
        }
    }

    virtual void stop() override {
        std::call_once(flag_, [this] { _stop(); });  // 多线程只调用一次
    }

    // XXX:这里不做不定参数的接口，外部传入时可以自行绑定
    virtual void add_task(const thread_task_t &task) override {
        queue_.push([task]() { task(); });
    }

private:
    void _stop(void) {
        queue_.stop();
        running_ = false;

        for (auto thread : threads_) {
            if (thread) {
                thread->join();
            }
        }

        threads_.clear();
    }

private:
    std::list<std::shared_ptr<std::thread>> threads_;  ///< 线程队列
    MicroSyncTaskQueue<thread_task_t> queue_;          ///< 线程任务队列
    std::atomic_bool running_;  ///< 线程池运行状态
    std::once_flag flag_;       ///< 标记
    std::mutex mutex_;          ///< 线程池锁
};

}
