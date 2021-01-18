/**
 * @file micro_kernel.hpp
 * @author wotsen (astralrovers@outlook.com)
 * @brief
 * @date 2021-01-15
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include <iostream>
#include <mutex>
#include <stdexcept>
#include "micro_thread_pool.hpp"
#include "plugin.hpp"

namespace Asty {

#define MICRO_KERNEL_VERSION "1.0.0"

/**
 * @brief 微内核
 * 
 * @tparam T 插件Key的类型
 */
template <typename T>
class MicroKernel : public IMicroKernelServices<T> {
public:
    MicroKernel(uint32_t plugin_limit, std::shared_ptr<IThreadPool> thread_pool)
        : version_(MICRO_KERNEL_VERSION),
          limit_(plugin_limit),
          thread_pool_(thread_pool),
          running_(false),
          exit_(false) {
        if (!thread_pool_) {
            throw std::invalid_argument("compare or thread_pool null");
        }
    }

    virtual ~MicroKernel() { stop(); }

    // 启动微内核
    void run(void) {
        std::unique_lock<std::mutex> lck(mtx_);

        if (running_) {
            return;
        }

        std::list<PluginKey<T>> bad_plugin;

        // 初始化
        for (auto &plugin : plugins_) {
            if (plugin.second) {
                plugin.second->set_micro_kernel_srv(this);
                if (!plugin.second->plugin_init()) {
                    plugin.second->set_plugin_status(E_PLUGIN_BAD);
                    std::cout << "plugin : [name = " << plugin.first.name
                              << "] [version = " << plugin.first.version
                              << "] init failed" << std::endl;
                    bad_plugin.push_front(plugin.first);
                }
            }
        }

        // 清除初始化异常的插件
        for (auto &item : bad_plugin) {
            auto k = plugins_.find(item);

            if (k == plugins_.end()) {
                plugins_.erase(k);
            }
        }

        bad_plugin.clear();

        // 启动插件
        for (auto &plugin : plugins_) {
            if (plugin.second) {
                if (!plugin.second->plugin_start()) {
                    plugin.second->set_plugin_status(E_PLUGIN_BAD);
                    std::cout << "plugin : [name = " << plugin.first.name
                              << "] [version = " << plugin.first.version
                              << "] start failed" << std::endl;
                    bad_plugin.push_front(plugin.first);
                }
                plugin.second->set_plugin_status(E_PLUGIN_RUNING);
            }
        }

        // 清除启动异常的插件
        for (auto &item : bad_plugin) {
            auto k = plugins_.find(item);

            if (k == plugins_.end()) {
                plugins_.erase(k);
            }
        }

        bad_plugin.clear();

        running_ = true;
        exit_ = false;

        lck.unlock();

        // 进入微内核循环
        while (running_) {
            // 整个插件循环一次才进行解锁
            lck.lock();

            // 循环添加任务到线程池进行执行
            for (auto plugin : plugins_) {
                // 判断一次退出
                if (!running_) {
                    lck.unlock();
                    goto __exit;
                }

                if (plugin.second->plugin_task_en()) {
                    thread_pool_->add_task(
                        [plugin] { plugin.second->plugin_task(); });
                }
            }

            lck.unlock();
        }

    __exit:

        exit_ = true;
        micro_kernel_exited_.notify_one();
    }

private:
    // 停止微内核
    void stop(void) {
        std::unique_lock<std::mutex> lck(mtx_);

        if (!running_) {
            return;
        }

        running_ = false;

        // 等待微内核退出
        micro_kernel_exited_.wait(lck, [this] { return exit_; });

        for (auto &plugin : plugins_) {
            if (E_PLUGIN_RUNING == plugin.second->plugin_status()) {
                plugin.second->plugin_stop();
                plugin.second->plugin_exit();
                plugin.second->set_plugin_status(E_PLUGIN_STOP);
            }
        }
    }

public:
    // 微内核版本
    virtual std::string micro_kernel_version(void) override { return version_; }

    // 插件数量
    virtual uint32_t plugin_cnt(void) override { return plugins_.size(); }
    // 插件注册
    bool plugin_register(std::shared_ptr<IPlugin<T>> plugin) {
        std::unique_lock<std::mutex> lck(mtx_);

        // 插件数量达到限制
        if (plugins_.size() >= limit_) {
            return false;
        }

        auto item = plugins_.find(plugin->plugin_key_);

        // 插件冲突
        if (item != plugins_.end()) {
            return false;
        }

        plugin->set_micro_kernel_srv(this);

        // 如果正在运行，则执行初始化和启动
        if (running_) {
            if (!plugin->plugin_init() || !plugin->plugin_start()) {
                return false;
            }
            plugin->set_plugin_status(E_PLUGIN_RUNING);
        }

        plugins_.insert(std::pair<PluginKey<T>, std::shared_ptr<IPlugin<T>>>(
            plugin->plugin_key_, plugin));

        return true;
    }
    // 插件注销
    bool plugin_unregister(const T &key) {
        std::unique_lock<std::mutex> lck(mtx_);

        PluginKey<T> plugin_key;
        plugin_key.key = key;

        auto item = plugins_.find(plugin_key);

        // 插件未找到
        if (item == plugins_.end()) {
            return false;
        }

        // 微内核未运行则直接返回
        if (!running_) {
            return true;
        }

        // 插件退出
        if (E_PLUGIN_RUNING == item->second->plugin_status()) {
            item->second->plugin_stop();
            item->second->plugin_exit();
            item->second->set_plugin_status(E_PLUGIN_STOP);
        }

        return true;
    }
    // 信息查询
    virtual bool plugin_key(const T &key, PluginKey<T> &item_key) override {
        std::unique_lock<std::mutex> lck(mtx_);

        PluginKey<T> tmp;
        tmp.key = key;

        auto item = plugins_.find(tmp);

        // 插件未找到
        if (item == plugins_.end()) {
            return false;
        }

        item_key = item->first;

        return true;
    }

    // 消息分发
    virtual bool message_dispatch(const PluginKey<T> &from, const T &to_key,
                                  const PluginDataT &request,
                                  PluginDataT &response) override {
        std::unique_lock<std::mutex> lck(mtx_);

        PluginKey<T> to;
        to.key = to_key;

        auto item = plugins_.find(to);

        // 插件未找到
        if (item == plugins_.end()) {
            return false;
        }

        // 重新赋值
        to = item->first;
        to.key = to_key;

        const PluginMessage<T> req_msg{from, to, request};

        PluginMessage<T> res_msg{to, (PluginKey<T>)from, response};

        // 插件消息处理
        bool ret = item->second->message(req_msg, res_msg);

        response = res_msg.data;

        return ret;
    }
    // 消息流分发
    virtual bool stream_dispatch(
        std::shared_ptr<IPluginStream<T>> stream) override {
        std::unique_lock<std::mutex> lck(mtx_);

        auto item = plugins_.find(stream->to_);

        // 插件未找到
        if (item == plugins_.end()) {
            return false;
        }

        // 重新赋值
        stream->to_.name = item->first.name;
        stream->to_.version = item->first.version;

        auto plugin = item->second;

        // 流式消息添加到线程池任务内去传递
        thread_pool_->add_task([=] { plugin->stream(stream); });

        return true;
    }

    // 日志
    virtual void log(const std::string &message) override {
        // TODO
    }

private:
    std::mutex mtx_;       ///< 操作锁，TODO:替换为读写锁
    std::string version_;  ///< 微内核版本
    uint32_t limit_;       ///< 插件数量限制
    std::map<PluginKey<T>, std::shared_ptr<IPlugin<T>>> plugins_;  ///< 插件列表
    std::shared_ptr<IThreadPool> thread_pool_;     ///< 线程池
    std::condition_variable micro_kernel_exited_;  ///< 微内核退出条件变量
    std::atomic_bool running_;                     ///< 微内核运行状态
    bool exit_;                                    ///< 微内核退出标记
};

}
