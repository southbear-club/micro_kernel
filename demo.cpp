/**
 * @file demo.cpp
 * @author wotsen (astralrovers@outlook.com)
 * @brief
 * @date 2021-01-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#include <string.h>
#include <iostream>
#include "micro_kernel.hpp"
#include "plugin.hpp"

using namespace Asty;

typedef enum : int {
    E_DOMAIN_BASIC = 0,
    E_DOMAIN_ALARM = 1,
} domain_type;

class PluginStream : public IPluginStream<domain_type> {
public:
    PluginStream(const PluginKey<domain_type> &from,
                 const PluginKey<domain_type> &to)
        : IPluginStream<domain_type>(from, to) {}

    // 关闭流式通信连接
    virtual void close() override { return; }

    // 检查连接是否关闭
    virtual bool is_closed(void) override { return false; }

    // 发送数据
    virtual int send(const PluginDataT &data, const time_t wait = -1) override {
        std::cout << "send.." << std::endl;
        return 0;
    }

    // 接收数据
    virtual int recv(PluginDataT &data, const time_t wait = -1) override {
        std::cout << "recv.." << std::endl;
        return 0;
    }

public:
    char buf[128];
};

class BasicPlugin : public IPlugin<domain_type> {
public:
    BasicPlugin(const PluginKey<domain_type> &key)
        : IPlugin<domain_type>(key) {}

    // 初始化
    virtual bool plugin_init(void) override {
        std::cout << "basic init" << std::endl;
        return true;
    }

    // 启动
    virtual bool plugin_start(void) override {
        std::cout << "basic start" << std::endl;
        return true;
    }

    // cycle调用
    virtual bool plugin_task(void) override {
        std::cout << "basic invok" << std::endl;
        domain_type t1 = E_DOMAIN_BASIC;
        domain_type t2 = E_DOMAIN_ALARM;
        PluginKey<domain_type> from{"basic", "1.0.0", E_DOMAIN_BASIC};

        PluginKey<domain_type> to;

        to.key = t2;

        char reqb[128] = "hello alarm";
        char resb[128] = "";
        PluginDataT req;
        PluginDataT res;

        req.len = strlen(reqb);
        req.data = reqb;

        res.len = sizeof(resb);
        res.data = resb;

        get_micro_kernel_service()->message_dispatch(from, t2, req, res);

        std::cout << "message back : " << (char *)res.data << std::endl;

        return true;
    }

    virtual bool plugin_task_en(void) override {
        return true;
    }

    // 停止
    virtual bool plugin_stop(void) override {
        std::cout << "basic stop" << std::endl;
        return true;
    }

    // 退出
    virtual bool plugin_exit(void) override {
        std::cout << "basic exit" << std::endl;
        return true;
    }

    // 消息通知，来自微内核
    virtual bool notice(const PluginDataT &msg) override {
        std::cout << "basic notice" << std::endl;
        return true;
    }

    // 消息处理
    virtual bool message(const PluginMessage<domain_type> &request,
                         PluginMessage<domain_type> &response) override {
        std::cout << "basic message" << std::endl;
        return true;
    }

    // 流式消息处理
    virtual bool stream(
        std::shared_ptr<IPluginStream<domain_type>> &stream) override {
        std::cout << "basic message" << std::endl;
        return true;
    }
};

class AlarmPlugin : public IPlugin<domain_type> {
public:
    AlarmPlugin(const PluginKey<domain_type> &key)
        : IPlugin<domain_type>(key) {}

    // 初始化
    virtual bool plugin_init(void) override {
        std::cout << "alarm init" << std::endl;
        return true;
    }

    // 启动
    virtual bool plugin_start(void) override {
        std::cout << "alarm start" << std::endl;
        return true;
    }

    // cycle调用
    virtual bool plugin_task(void) override {
        std::cout << "alarm invok : [type = " << (int)plugin_key().key << "]" << std::endl;
        return true;
    }

    virtual bool plugin_task_en(void) override {
        return true;
    }

    // 停止
    virtual bool plugin_stop(void) override {
        std::cout << "alarm stop" << std::endl;
        return true;
    }

    // 退出
    virtual bool plugin_exit(void) override {
        std::cout << "alarm exit" << std::endl;
        return true;
    }

    // 消息通知，来自微内核
    virtual bool notice(const PluginDataT &msg) override {
        std::cout << "alarm notice" << std::endl;
        return true;
    }

    // 消息处理
    virtual bool message(const PluginMessage<domain_type> &request,
                         PluginMessage<domain_type> &response) override {
        std::cout << "alarm message, from : " << request.from.name
                  << ", msg : " << (char *)request.data.data << std::endl;

        sprintf((char *)response.data.data, "hihi basic");
        response.data.len = strlen("hihi basic");

        return true;
    }

    // 流式消息处理
    virtual bool stream(
        std::shared_ptr<IPluginStream<domain_type>> &stream) override {
        std::cout << "alarm message" << std::endl;
        return true;
    }
};

int main(void) {
    std::shared_ptr<MicroKernelThreadPool> thread_pool(
        new MicroKernelThreadPool);
    std::shared_ptr<MicroKernel<domain_type>> micro_kernel(
        new MicroKernel<domain_type>(200, thread_pool));

    PluginKey<domain_type> basic_key{"basic", "1.0.0", E_DOMAIN_BASIC};

    std::shared_ptr<BasicPlugin> basic(new BasicPlugin(basic_key));
    micro_kernel->plugin_register(basic);

    PluginKey<domain_type> alarm_key{"basic", "1.0.0", E_DOMAIN_ALARM};
    for (int i = 1; i < 100; i++) {
        alarm_key.key = (domain_type)i;
        std::shared_ptr<AlarmPlugin> alarm(new AlarmPlugin(alarm_key));
        micro_kernel->plugin_register(alarm);
    }

    micro_kernel->run();

    return 0;
}