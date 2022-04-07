#ifndef _SYSTEM_START_GUI_H
#define _SYSTEM_START_GUI_H
#include "stdint.h"

void system_start_gui_init(void);
void system_start_display_init();
void system_start_display_bar(uint8_t progress);
void system_start_display_label(char *str, uint32_t c);

enum SYS_START_INFO{
    BSP_INITING = 0,
    BSP_INIT_OK,
    BSP_INIT_FAIL,
    NET_WAIT_CONNECT,
    NET_WAIT_AIRKISS,
    NET_SUCCESS,
    NET_FAIL,
    SYS_WAIT_INFO_UPDATE,
};
#endif
