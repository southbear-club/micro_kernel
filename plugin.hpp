/**
 * @file plugin.hpp
 * @author wotsen (astralrovers@outlook.com)
 * @brief
 * @date 2021-01-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include <time.h>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace Asty {

// 插件抽象类
template <typename T>
class IPlugin;
// 微内核类
template <typename T>
class MicroKernel;

/**
 * @brief 插件key
 * 
 * @tparam T key类型，由用户自行定义
 */
template <typename T>
struct PluginKey {
    PluginKey() {}
    PluginKey(const std::string &name, const std::string &version, const T &key)
        : name(name), version(version), key(key) {}
    PluginKey(const PluginKey<T> &key)
        : name(key.name), version(key.version), key(key.key) {}

    bool operator==(const PluginKey<T> &keyn) const { return key == keyn.key; }

    bool operator>(const PluginKey<T> &keyn) const { return key > keyn.key; }

    bool operator<(const PluginKey<T> &keyn) const { return key < keyn.key; }

    std::string name;     ///< 插件名称
    std::string version;  ///< 插件版本
    T key;  ///< 插件key，由用户自己的业务定义，确保每个插件唯一，且需要实现==,
            ///< >, <运算符重载
};

/**
 * @brief 插件通信数据格式，微内核仅透传
 *
 */
struct PluginDataT {
    int type;    ///< 数据类型，由插件实现自行定义
    int len;     ///< 数据长度
    void *data;  ///< 数据实体
};

/**
 * @brief 插件通信消息格式，仅用于短连接
 *
 * @tparam T key类型
 */
template <typename T>
struct PluginMessage {
    PluginKey<T> from;  ///< 源插件
    PluginKey<T> to;    ///< 目的插件
    PluginDataT data;   ///< 通信数据
};

/**
 * @brief 插件通信消息格式，用于长连接
 *
 * @tparam T key类型
 */
template <typename T>
class IPluginStream {
public:
    // 插件在创建stream时，to这个对象，只需要填充其中的key，name和version由微内核填充
    IPluginStream(const PluginKey<T> &from, const PluginKey<T> &to)
        : from_(from), to_(to) {}
    virtual ~IPluginStream() {}

    // 关闭流式通信连接
    virtual void close() = 0;
    // 检查连接是否关闭
    virtual bool is_closed(void) = 0;
    // 发送数据
    virtual int send(const PluginDataT &data, const time_t wait = -1) = 0;
    // 接收数据
    virtual int recv(PluginDataT &data, const time_t wait = -1) = 0;

public:
    PluginKey<T> from_;  ///< 源插件
    PluginKey<T> to_;    ///< 目的插件
};

/**
 * @brief 插件运行状态
 *
 */
typedef enum {
    E_PLUGIN_STOP = 0,    ///< 停止
    E_PLUGIN_RUNING = 1,  ///< 正在运行
    E_PLUGIN_BAD = 2,     ///< 异常
} plugin_run_status;

/**
 * @brief 微内核服务接口类，提供给插件使用
 * @tparam T key类型
 */
template <typename T>
class IMicroKernelServices {
public:
    virtual ~IMicroKernelServices() {}

    // 微内核版本
    virtual std::string micro_kernel_version(void) = 0;
    // 插件数量
    virtual uint32_t plugin_cnt(void) = 0;
    // 插件信息查询，不允许在init start stop exit插件接口内同步调用，否则会造成微内核死锁
    virtual bool plugin_key(const T &key, PluginKey<T> &item_key) = 0;

    // 消息分发
    virtual bool message_dispatch(const PluginKey<T> &from, const T &to_key,
                                  const PluginDataT &request,
                                  PluginDataT &response) = 0;
    // 消息流分发
    virtual bool stream_dispatch(std::shared_ptr<IPluginStream<T>> stream) = 0;

    // 日志
    virtual void log(const std::string &message) = 0;
};

/**
 * @brief 插件抽象类，用户实现时基于此接口
 * @warning 所有接口实现不得死循环
 * @tparam T key类型
 */
template <typename T>
class IPlugin {
public:
    IPlugin(const PluginKey<T> &key)
        : plugin_key_(key),
          plugin_st_(E_PLUGIN_STOP),
          mic_kernel_srv_(nullptr) {}

    virtual ~IPlugin() {}

    // 获取插件信息
    const PluginKey<T> &plugin_key(void) const { return plugin_key_; }

    // 获取插件状态
    plugin_run_status plugin_status(void) { return plugin_st_; }

    // 获取微内核服务
    IMicroKernelServices<T> *get_micro_kernel_service(void) {
        return mic_kernel_srv_;
    }

    // 继续运行
    bool continue_run(void) {
        // TODO
        return true;
    }

    // 暂停运行
    bool pause_run(void) {
        // TODO
        return true;
    }

    // 注册
    bool register_self(IMicroKernelServices<T> *micro_kernel) {
        // TODO
        return true;
    }

    // 注销
    bool unregister_self(void) {
        // TODO
        return true;
    }

public:
    // 初始化
    virtual bool plugin_init(void) = 0;

    // 启动
    virtual bool plugin_start(void) = 0;

    // 微内核cycle调用，每次微内核循环都会添加到线程池任务，因此该接口实现时不得死循环
    virtual bool plugin_task(void) = 0;

    // 如果插件实现不需要微内核循环去添加插件任务，而是开启自己的插件线程，那么建议该接口返回false，用于避免不必要的任务添加，增强微内核性能
    virtual bool plugin_task_en(void) = 0;

    // 停止
    virtual bool plugin_stop(void) = 0;

    // 退出
    virtual bool plugin_exit(void) = 0;

    // 消息通知，来自微内核
    virtual bool notice(const PluginDataT &msg) = 0;

    // 消息处理
    virtual bool message(const PluginMessage<T> &request,
                         PluginMessage<T> &response) = 0;

    // 流式消息处理
    virtual bool stream(std::shared_ptr<IPluginStream<T>> stream) = 0;

private:
    friend class MicroKernel<T>;
    // 设置插件状态
    void set_plugin_status(plugin_run_status st) { plugin_st_ = st; }

    // 设置微内核服务
    void set_micro_kernel_srv(IMicroKernelServices<T> *mic_kernel_srv) {
        mic_kernel_srv_ = mic_kernel_srv;
    }

private:
    PluginKey<T> plugin_key_;                  ///< 插件信息
    plugin_run_status plugin_st_;              ///< 插件状态
    IMicroKernelServices<T> *mic_kernel_srv_;  ///< 微内核服务
};

}
