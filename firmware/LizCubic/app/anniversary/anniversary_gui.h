#ifndef APP_ANNIVERSARY_GUI_H
#define APP_ANNIVERSARY_GUI_H

#include "lvgl.h"
#include "sys/time.h"

void anniversary_gui_init(void);
void anniversary_gui_load(lv_scr_load_anim_t anim_type);
void anniversary_gui_display_date(struct tm* target);
extern int anniversary_day_count;
#endif
