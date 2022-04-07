/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-16     chenyong     first version
 */

#ifndef __AT_DEVICE_SIM76XX_H__
#define __AT_DEVICE_SIM76XX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include <at_device.h>

/* The maximum number of sockets supported by the sim76xx device */
#define AT_DEVICE_SIM76XX_SOCKETS_NUM      10

enum AT_NETDEV_LTE_NOTIF_EVENT
{
    LTE_SIM_FAIL = 0, //SIM卡校验不成功
    LTE_CSQ_FAIL, //信号太差
    LTE_NET_FAIL,  // 网络失败
    LTE_NET_SUCCESS, // 网络连接
    LTE_INIT_DONE, // 初始化完成
};

struct at_device_sim76xx
{
    char *device_name;
    char *client_name;
    int power_pin;
    int power_status_pin;
    size_t recv_line_num;
    void (*notification)(enum AT_NETDEV_LTE_NOTIF_EVENT event);

    struct at_device device;
    int linkInd;
    void *user_data;
};

#ifdef AT_USING_SOCKET

/* sim76xx device socket initialize */
int sim76xx_socket_init(struct at_device *device);

/* sim76xx device class socket register */
int sim76xx_socket_class_register(struct at_device_class *class);
int sim76xx_ntp_sync();
#endif /* AT_USING_SOCKET */

#ifdef __cplusplus
}
#endif

#endif /* __AT_DEVICE_SIM76XX_H__ */
