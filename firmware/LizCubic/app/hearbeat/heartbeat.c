#include "heartbeat_gui.h"
#include "interface.h"
#include "rtthread.h"
#include "heartbeat.h"
#include "liz_mqtt.h"
#include "liz_bsp.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app.heartbeat"
#if defined(LIZ_APP_HEARTBEAT_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>
#define INTER_ENTER 0
#define USER_ENTER 1
static void heartbeat_init();
static void heartbeat_load(lv_scr_load_anim_t anim_type);
static void process(struct APP_CONTROLLER *sys, struct _app_action_info *act_info);
static void exit_callback();

static uint8_t send_cnt = 0;
static uint8_t recv_cnt = 0;
static uint8_t enter_type = 0;
static uint8_t is_init = 0;
static uint8_t need_del_inter = 0;
static void heartbeat_init()
{
    if (is_init) return;
    is_init = 1;
    heartbeat_gui_init(); // gui初始化
}

static void heartbeat_load(lv_scr_load_anim_t anim_type)
{
    heartbeat_gui_load(anim_type);
    enter_type = (anim_type == LV_SCR_LOAD_ANIM_MOVE_BOTTOM) ? INTER_ENTER : USER_ENTER; //中断进来标志中断进来
    if (enter_type == INTER_ENTER) // 为1 时，表示是因为中断进来
    {
        send_cnt = 0;
        recv_cnt = 1;
        /* 呼吸效果 表明来了消息 */
        need_del_inter = 1; // 处于中断通知中
        liz_bsp.rgbled->set_color(0,0x324567);
        liz_bsp.rgbled->set_breathe_period(20);
        liz_bsp.rgbled->set_mode(RGB_LED_MODE_BREATHE);
    }
    else if (enter_type == USER_ENTER) // 用户手动进来
    {
        send_cnt = 1;
        recv_cnt = 0;
        liz_mqtt_publish(SEND_MSG, sizeof(SEND_MSG)); //加载页面时候便发送一条消息
    }
}

static void process(struct APP_CONTROLLER *sys, struct _app_action_info *act_info)
{

}

static void exit_callback()
{
    heartbeat_gui_unload();
}

static void on_event(int event_id)
{
    switch (event_id)
    {
    case 1: // 人为进入
        liz_mqtt_publish(SEND_MSG, sizeof(SEND_MSG)); // 手动发送消息
        send_cnt++;
        if(need_del_inter) //如果处于中断通知中
        {
            need_del_inter = 0;
            // 取消呼吸灯效果
            liz_bsp.rgbled->set_mode(RGB_LED_MODE_FADEOUT);
        }
        else // 不在中断通知中
        {
            // 发送指示灯
            liz_bsp.rgbled->set_color(0,0x700000);
            liz_bsp.rgbled->set_breathe_period(8);
            liz_bsp.rgbled->set_mode(RGB_LED_MODE_FADEOUT);
        }
        if (recv_cnt > 0) // 已经收到过了
        {
            heartbeat_set_sr_type(HEART);
        }
        LOG_D("send heartbeat");
        break;
    
    case 2: // 中断进入
        if (send_cnt > 0)   //已经手动发送过了
        {
            heartbeat_set_sr_type(HEART);
        }
        if(!need_del_inter) // 处于非中断通知模式下
        {
            /* 亮一下 */
            liz_bsp.rgbled->set_color(0,0x324567);
            liz_bsp.rgbled->set_breathe_period(8);
            liz_bsp.rgbled->set_mode(RGB_LED_MODE_FADEOUT);
        }
        recv_cnt++;
        LOG_D("received heartbeat");
        break;

    default:
        break;
    }
    heartbeat_set_send_recv_cnt_label(send_cnt, recv_cnt);
}

struct APP_OBJ heartbeat_app =
{
    .name = "heartbeat",
    .image = RT_NULL,
    .info = "",
    .init = heartbeat_init,
    .load = heartbeat_load,
    .process = process,
    .exit_callback = exit_callback,
    .get_parm = RT_NULL,
    .on_event = on_event,
};


