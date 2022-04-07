#include "lvgl.h"
#include "liz_mqtt.h"
#include "heartbeat_gui.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app-gui.heartbeat"
#if defined(LIZ_APP_HEARTBEAT_DEBUG)
#define DBG_LVL    DBG_LOG
#else 
#define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

static uint8_t is_running = 0; // app是否还在运行

LV_FONT_DECLARE(jb_monob_yahei_22);

static lv_style_t default_style;
static lv_style_t chFont_style;

static lv_obj_t *scr = NULL;
static lv_obj_t *heartbeatImg = NULL;
static lv_obj_t *txtlabel = NULL;

lv_timer_t * heartbeat_update_timer = NULL;
static void heartbeat_update_cb(lv_timer_t * timer);
static enum S_R_TYPE s_r_type = SEND;
static uint8_t txt_type = 0;

#define TEXT_TYPE0_FMT "已发送#00ff45 %02d#次,接收到#ff0000 %02d#次"
#define TEXT_TYPE1_FMT "接收到#ff0000 %02d#次,已发送#00ff45 %02d#次"

#define RECV_IMG_PATH "S:/archerr/archerr%d.bin"
#define SEND_IMG_PATH "S:/archers/archers%d.bin"
#define HEART_IMG_PATH "S:/heart/heart%d.bin"

#define HEART_IMG_NUM 20
#define RECV_IMG_NUM 10
#define SEND_IMG_NUM 9

void heartbeat_gui_init(void)
{
    lv_style_reset(&default_style);
    lv_style_init(&default_style);
    lv_style_set_bg_opa(&default_style,LV_OPA_COVER);
    lv_style_set_bg_color(&default_style,  lv_color_black());

    lv_style_reset(&chFont_style);
    lv_style_init(&chFont_style);
    lv_style_set_text_opa(&chFont_style,LV_OPA_COVER);
    lv_style_set_text_color(&chFont_style, lv_color_white());
    lv_style_set_text_font(&chFont_style, &jb_monob_yahei_22);     
}

void heartbeat_gui_load(lv_scr_load_anim_t anim_type)
{
    is_running = 0;
    scr = lv_obj_create(NULL);
    lv_obj_add_style(scr,&default_style,0);
    char img_path[30] ={0};
    heartbeatImg = lv_img_create(scr); //创建heart图标

    txtlabel = lv_label_create(scr);
    lv_obj_add_style(txtlabel,&chFont_style,0);
    lv_label_set_recolor(txtlabel, true);

    if(anim_type == LV_SCR_LOAD_ANIM_MOVE_TOP) // 手动发送进入的
    {
        txt_type = 0;
        lv_label_set_text_fmt(txtlabel, TEXT_TYPE0_FMT, 1, 0);
        rt_snprintf(img_path,24,SEND_IMG_PATH,0);
        s_r_type = SEND;
    }
    else // 接收到消息进入的
    {
        txt_type = 1;
        lv_label_set_text_fmt(txtlabel, TEXT_TYPE1_FMT, 1, 0);
        rt_snprintf(img_path,24,RECV_IMG_PATH,0);
        s_r_type = RECV;
    }  
    lv_img_set_src(heartbeatImg,img_path);

    lv_obj_align(scr,LV_ALIGN_CENTER,0,0);
    lv_obj_align(heartbeatImg,LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(txtlabel,LV_ALIGN_TOP_MID, 0, 15);
    // lv_obj_align(recv_cnt_label,LV_ALIGN_TOP_RIGHT, -20, 15);
    // lv_obj_align(send_cnt_label,LV_ALIGN_TOP_LEFT, 20, 15);

    lv_scr_load_anim(scr, anim_type, 300, 150, true);
    while(lv_anim_count_running()!=0) //当前页面有长文本滚动动画
    {
        lv_task_handler();
        rt_thread_mdelay(100);
    }

    heartbeat_update_timer = lv_timer_create(heartbeat_update_cb, 100, NULL);
    lv_timer_set_repeat_count(heartbeat_update_timer, -1);
    lv_timer_ready(heartbeat_update_timer);
    is_running = 1;
}

void display_heartbeat(void)
{
    static int _beatIndex = 0;
    char buf[30] = {0};
    if (NULL != scr && lv_scr_act() == scr)
    {
        if(s_r_type==SEND)
        {
            _beatIndex = (_beatIndex + 1) % SEND_IMG_NUM; 
            rt_snprintf(buf,26,SEND_IMG_PATH,_beatIndex);     
        }
        else if(s_r_type==RECV)
        {
            _beatIndex = (_beatIndex + 1) % RECV_IMG_NUM;
            rt_snprintf(buf,24,RECV_IMG_PATH,_beatIndex);
        }
        else
        {
            _beatIndex = (_beatIndex + 1) % HEART_IMG_NUM;
            rt_snprintf(buf, 24, HEART_IMG_PATH, _beatIndex); 
        }
        lv_img_set_src(heartbeatImg,buf);  
    }
}

void heartbeat_set_sr_type(enum S_R_TYPE type)
{
    s_r_type = type;
}

void heartbeat_set_send_recv_cnt_label(uint8_t send_num, uint8_t recv_num)
{
    if(txt_type)
        lv_label_set_text_fmt(txtlabel, TEXT_TYPE1_FMT, recv_num, send_num);
    else
        lv_label_set_text_fmt(txtlabel, TEXT_TYPE0_FMT, send_num, recv_num);
}

void heartbeat_set_recv_cnt_label(uint8_t num)
{
    // lv_label_set_text_fmt(recv_cnt_label, RECV_TEXT_FMT, num); 
}

static void heartbeat_update_cb(lv_timer_t * timer)
{
    if(is_running)
    {
        display_heartbeat();
    }   
}

void heartbeat_gui_unload()
{
    is_running = 0;
    lv_timer_del(heartbeat_update_timer);
    scr = NULL;
}


