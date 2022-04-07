#include "lvgl.h"
#include "system_start_gui.h"
#include "liz_config.h"
#include "network.h"


LV_FONT_DECLARE(jb_monob_22);

static lv_style_t default_style;
static lv_style_t bar_style;
static lv_style_t chFont_style;

static lv_obj_t *scr = NULL;
static lv_obj_t *label= NULL;
static lv_obj_t *start_bar= NULL;
/**
* @brief 系统开机GUI
* @param None
* @return None
*/
void system_start_gui_init(void)
{

    lv_style_reset(&default_style);
    lv_style_init(&default_style);
    lv_style_set_bg_opa(&default_style,LV_OPA_COVER);
    lv_style_set_bg_color(&default_style,  lv_color_black());

    lv_style_reset(&chFont_style);
    lv_style_init(&chFont_style);
    lv_style_set_text_opa(&chFont_style,LV_OPA_COVER);
    lv_style_set_text_color(&chFont_style, lv_color_white());
    lv_style_set_text_font(&chFont_style, &jb_monob_22); // consolab_22

    lv_style_reset(&bar_style);
    lv_style_init(&bar_style);
    lv_style_set_bg_color(&bar_style, lv_color_black());
    lv_style_set_border_width(&bar_style, 2);
    lv_style_set_border_color(&bar_style, lv_color_white());
    lv_style_set_pad_top(&bar_style, 1); //指示器到背景四周的距离
    lv_style_set_pad_bottom(&bar_style, 1);
    lv_style_set_pad_left(&bar_style, 1);
    lv_style_set_pad_right(&bar_style, 1);
}
void system_start_display_init()
{
    lv_obj_t *act_obj = lv_scr_act(); 
    scr = lv_obj_create(NULL);
    lv_obj_add_style(scr,&default_style,0);
    start_bar = lv_bar_create(scr);
    lv_obj_add_style(start_bar,&bar_style,0);
    lv_bar_set_range(start_bar, 0, 150);   
    lv_obj_set_size(start_bar, 180, 20);
	lv_obj_set_style_bg_color(start_bar, lv_color_make(0xFF, 0xFF, 0xFF), LV_STATE_DEFAULT | LV_PART_INDICATOR);
    lv_bar_set_value(start_bar, 0, LV_ANIM_OFF);

    /* 创建启动信息标签 */
    label = lv_label_create(scr); 
    lv_obj_add_style(label,&chFont_style,LV_STATE_DEFAULT);
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, "bsp initing...");

    lv_obj_align(start_bar, LV_ALIGN_CENTER, 0, -8);
    lv_obj_align(label,LV_ALIGN_CENTER,0,34);

    lv_scr_load(scr);
}
void system_start_display_bar(uint8_t progress)
{
    lv_bar_set_value(start_bar, progress, LV_ANIM_ON);
}


void system_start_display_label(char *str, uint32_t c)
{
    lv_label_set_text(label, str);
    
}