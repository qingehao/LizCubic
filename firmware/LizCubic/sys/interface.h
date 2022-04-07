#ifndef INTERFACE_H
#define INTERFACE_H
#include "app_controller.h"
#include "stdint.h"
#include "lvgl.h"
#include "imu_action.h"

enum APP_EVENT_TYPE
{
    APP_EVENT_WIFI_CONN = 0, // 开启连接
    APP_EVENT_WIFI_AP,       // 开启AP事件
    APP_EVENT_WIFI_ALIVE,    // wifi开关的心跳维持
    APP_EVENT_WIFI_DISCONN,  // 连接断开
    APP_EVENT_UPDATE_TIME,
    APP_EVENT_NONE
};

struct APP_OBJ
{
    const char *name;  // 应用程序名称 及title
    const void *image; // APP的图片存放地址    APP应用图标 128*128
    const char *info;  // 应用程序的其他信息 如作者、版本号等等
    void (*init)();    // APP的初始化函数 也可以为空或什么都不做（作用等效于arduino setup()函数）
    void (*load)(lv_scr_load_anim_t anim_type);    // APP的界面加载函数
    void (*process)(struct APP_CONTROLLER *sys,struct _app_action_info *act_info);    // APP的主程序函数入口指针
    void (*exit_callback)();                             // 退出之前需要处理的回调函数 可为空
    void (*get_parm)(void *arg); //获取应用程序的一些状态参数
    void (*on_event)(int event_id);
};

#endif