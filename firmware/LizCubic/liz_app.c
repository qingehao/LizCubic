#include "rtthread.h"
#include "lvgl.h"

#include "bsp/liz_bsp.h"
#include "bsp/IMU/imu_action.h"

#include "network/liz_mqtt.h"
#include "network/network.h"

#include "sys/app_controller.h"
#include "sys/system_start_gui.h"
#include "sys/ntp.h"

#include "app/weather/weather.h"
#include "app/spectrum/spectrum.h"
#include "app/hearbeat/heartbeat.h"
#include "app/anniversary/anniversary.h"

#include "lizCubic.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app"
#if defined(LIZ_APP_DEBUG)
#define DBG_LVL    DBG_LOG
#else 
#define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>
#define DEBUG_MODE_SPECTURM 0
#define DEBUG_MODE_NORMAL 1
#define DEBUG_MODE 1
struct APP_CONTROLLER *app_controller = RT_NULL;
static rt_thread_t liz_app_tid = RT_NULL;
static rt_thread_t lvgl_tid = RT_NULL;
static uint8_t system_start_flag = 0;
static uint8_t inter_flag = 0;
extern uint8_t network_link_status;
extern enum NETWORK_INIT_STATUS network_status;
struct _app_action_info act_info;

static void liz_app_thread(void *arg);
static void wait_system_start(uint8_t *progress,uint8_t finish_progress);
static void wait_first_app_update_finish(uint8_t *progress,uint8_t finish_progress);
static void mqtt_sub_cb(char *recv_buf,uint8_t len);

static void lvgl_thread(void *parameter)
{
    while(1)
    {
        lv_task_handler();
        rt_thread_mdelay(10);
    }
}
static void liz_init_thread()
{
    int ret = 0;
    system_start_gui_init();
    system_start_display_init();
    system_start_display_label("bsp initing",0xffffff);
    ret = liz_bsp_init();
    system_start_display_label("bsp init ok",0xffffff);
#if DEBUG_MODE != DEBUG_MODE_SPECTURM
    liz_network_init();
    while(network_status != NETWORK_INIT_STATUS_DONE)
    // while(1)
    {
        switch(network_status)
        {
            case NETWORK_INIT_STATUS_INITING:
            system_start_display_label("lte initing...",0xffffff);
            break;
            case NETWORK_INIT_STATUS_SIM_FAIL:
            system_start_display_label("checking sim...",0xffffff);
            break;
            case NETWORK_INIT_STATUS_CSQ_FAIL:
            system_start_display_label("checking signal...",0xffffff);
            break;
            case NETWORK_INIT_STATUS_NET_FAIL:
            system_start_display_label("checking net...",0xffffff);
            break;
            case NETWORK_INIT_STATUS_SUCCESS:
            system_start_display_label("success",0xffffff);
            break;
            default :break;
        }
        rt_thread_mdelay(200);
    }
    if(network_link_status)
    {
        system_start_display_label("weather updating...",0xffffff);
        ntp_auto_sync_init();
    }
    else
    {
        system_start_display_label("network connect fail",0xffffff);
    }
#endif   
    system_start_flag = 1;
    
}

void liz_app_init()
{
    rt_thread_t tid=rt_thread_create("liz_init",liz_init_thread,RT_NULL,1024,2,10);
    if(tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    lvgl_tid = rt_thread_create("lvgl", lvgl_thread, RT_NULL, LV_THREAD_STACK_SIZE, LV_THREAD_PRIO, 0);
    if(lvgl_tid != RT_NULL)
    {
        rt_thread_startup(lvgl_tid);
    }
    liz_app_tid = rt_thread_create("liz",liz_app_thread,RT_NULL,
                                    LIZ_APP_THREAD_STACK_SIZE,
                                    LIZ_APP_THREAD_PRIO,10);
    if(liz_app_tid != RT_NULL)
    {
        rt_thread_startup(liz_app_tid);
    }
}

static void liz_app_thread(void *arg)
{
    int init_progress = 0;
    /* 等待系统启动完毕 */
    wait_system_start(&init_progress,80);
    /* 初始化appcontroller */
    app_controller = app_controller_get(); // 获得app控制器地址
#if DEBUG_MODE != DEBUG_MODE_SPECTURM
    if(app_controller->app_register(&weather_app) != 0)
    {
        LOG_E("weather app register fail");
    }
#endif
    if(app_controller->app_register(&spectrum_app) != 0)
    {
        LOG_E("spectrum app register fail");
    }
    if(app_controller->app_register(&anniversary_app) != 0)
    {
        LOG_E("anniversary app register fail");
    }
    if(app_controller->app_register(&heartbeat_app) != 0)
    {
        LOG_E("heartbeat app register fail");
    }
    app_controller->init();
#if DEBUG_MODE != DEBUG_MODE_SPECTURM
    if(network_link_status)
        wait_first_app_update_finish(&init_progress,150);
#endif
    /* 加载第一个app */
    app_controller->load();
    act_info.act_type = liz_bsp.get_act_info();

    while(1)
    {
        act_info.act_type = liz_bsp.get_act_info();
        if(act_info.is_valid != 1) //无效状态
        {
            act_info.duration ++ ;
            if(act_info.duration>12) //超过400ms
            {
                act_info.is_valid = 1;
                act_info.duration = 0;
            }
        }
        app_controller->main_process(&act_info);
        rt_thread_mdelay(40);
    }
}
/**
 * @brief 等待系统启动 包括bsp(温湿度、IMU)初始化，WiFi连接
 * @param progress 当前进度
 * @return None
 */
static void wait_system_start(uint8_t *progress,uint8_t finish_progress)
{
    while(1)
    {
        system_start_display_bar(*progress);
        *progress += 1 ;
        if(*progress >=finish_progress) *progress = finish_progress;
        if(system_start_flag)
        {
            break;
        }
        rt_thread_mdelay(50);
    }
}
/**
 * @brief 等待第一个app的信息加载完毕
 * @param progress 当前进度
 * @return None
 */
static void wait_first_app_update_finish(uint8_t *progress,uint8_t finish_progress)
{
    uint8_t info_bitmap;
    while(1)
    {
        system_start_display_bar(*progress);
        *progress += 1 ;
        if(*progress >=finish_progress) *progress = finish_progress;
        app_controller->get_cur_app_parm(&info_bitmap);
        if(info_bitmap == 0x05)
        {
            break;
        }
        rt_thread_mdelay(50);
    }
}

