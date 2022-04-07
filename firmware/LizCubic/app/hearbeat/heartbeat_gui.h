#ifndef APP_HEARTBEAT_GUI_H
#define APP_HEARTBEAT_GUI_H
#include "lvgl.h"
#include "stdint.h"

enum S_R_TYPE{
    SEND = 0,
    RECV,
    HEART,
};

void heartbeat_gui_init(void);
void heartbeat_gui_load(lv_scr_load_anim_t anim_type);
void heartbeat_gui_unload();

void display_heartbeat(void);

void heartbeat_set_sr_type(enum S_R_TYPE type);

void heartbeat_set_send_recv_cnt_label(uint8_t send_num, uint8_t recv_num);

#endif
