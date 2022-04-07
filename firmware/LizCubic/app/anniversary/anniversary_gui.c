#include "anniversary_gui.h"
#include "lvgl.h"
#include "sys/time.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app-gui.anniversary"
#if defined(LIZ_APP_ANNIVERSARY_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

LV_FONT_DECLARE(lv_font_ibmplex_115);
LV_FONT_DECLARE(msyhbd_18);
LV_FONT_DECLARE(msyhbd_22);
LV_FONT_DECLARE(msyhbd_24);

static uint8_t is_running = 0;
int anniversary_day_count = 0;
static lv_style_t default_style;
static lv_style_t chFont_style;
static lv_style_t numberBig_style;
static lv_style_t smallch_style;
static lv_style_t bigch_style;
static lv_style_t btn_style;

static lv_obj_t *scr = NULL;
static lv_obj_t *txtLabel = NULL; // 文本提示信息
static lv_obj_t *daycont = NULL;
static lv_obj_t *dayLabel = NULL; // 在一起天数
static lv_obj_t *btn = NULL, *btnLabel = NULL;
static lv_obj_t *targetDateLabel = NULL; //目标日期

#define TARGET_DATE_LOVE "2022年1月1日"
extern struct tm target_date;
static const char weekDayCh[7][4] = {"日", "一", "二", "三", "四", "五", "六"};

void anniversary_gui_init(void)
{
    lv_style_reset(&default_style);
    lv_style_init(&default_style);
    lv_style_set_bg_opa(&default_style, LV_OPA_COVER);
    lv_style_set_bg_color(&default_style,  lv_color_black());
    lv_style_set_border_width(&default_style,0);

    lv_style_reset(&chFont_style);
    lv_style_init(&chFont_style);
    lv_style_set_text_opa(&chFont_style, LV_OPA_COVER);
    lv_style_set_text_color(&chFont_style, lv_color_white());
    lv_style_set_text_font(&chFont_style, &msyhbd_22);

    lv_style_reset(&bigch_style);
    lv_style_init(&bigch_style);
    lv_style_set_text_opa(&bigch_style, LV_OPA_COVER);
    lv_style_set_text_color(&bigch_style, lv_color_white());
    lv_style_set_text_font(&bigch_style, &msyhbd_24);

    lv_style_reset(&smallch_style);
    lv_style_init(&smallch_style);
    lv_style_set_text_opa(&smallch_style, LV_OPA_COVER);
    lv_style_set_text_color(&smallch_style, lv_color_white());
    lv_style_set_text_font(&smallch_style, &msyhbd_18);

    lv_style_reset(&numberBig_style);
    lv_style_init(&numberBig_style);
    lv_style_set_text_opa(&numberBig_style, LV_OPA_COVER);
    lv_style_set_text_color(&numberBig_style, lv_color_white());
    lv_style_set_text_font(&numberBig_style, &lv_font_ibmplex_115);

    lv_style_reset(&btn_style);
    lv_style_init(&btn_style);
    lv_style_set_border_width(&btn_style,  0);
}

void anniversary_gui_load(lv_scr_load_anim_t anim_type)
{
    is_running = 0;
    scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &default_style, 0);

    /* 创建文本消息标签 */
    txtLabel = lv_label_create(scr);
    lv_obj_add_style(txtLabel, &bigch_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(txtLabel, "我们在一起");

    /* 创建天数标签 */
    dayLabel = lv_label_create(scr);
    lv_obj_add_style(dayLabel, &numberBig_style, 0);
    lv_label_set_recolor(dayLabel, true);
    lv_label_set_text_fmt(dayLabel, "#ffa500 %02d#", anniversary_day_count);

    /* 创建天了/天后btn */
    btn = lv_btn_create(scr);
    lv_obj_add_style(btn, &btn_style, LV_PART_MAIN);
    lv_obj_set_size(btn, 46, 26);
    lv_obj_set_style_bg_color(btn, lv_color_make(0xFF, 0xA5, 0x00), LV_STATE_DEFAULT | LV_PART_MAIN);

    /* 创建天了/天后btnLable */
    btnLabel = lv_label_create(btn);
    lv_obj_add_style(btnLabel, &smallch_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(btnLabel, "天了");

    /* 创建目标日标签 */
    targetDateLabel = lv_label_create(scr);
    lv_obj_add_style(targetDateLabel, &chFont_style, 0);
    lv_label_set_recolor(targetDateLabel, true);
    lv_label_set_text_fmt(targetDateLabel, "#FFFFFF %d年%d月%d日  星期%s#", target_date.tm_year, target_date.tm_mon, target_date.tm_mday,
                                                                            weekDayCh[target_date.tm_wday]);

    /* 创建设置对齐方式 */
    lv_obj_align(scr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(txtLabel, LV_ALIGN_CENTER, 0, -70);

    lv_obj_align(dayLabel, LV_ALIGN_CENTER, -10, 20);
    lv_obj_align_to(btn,dayLabel,LV_ALIGN_OUT_RIGHT_TOP,5,5);
    // lv_obj_align(btn, LV_ALIGN_CENTER, 50 , -20);
    lv_obj_align(btnLabel, LV_ALIGN_CENTER, 0, -1);

    lv_obj_align(targetDateLabel, LV_ALIGN_CENTER, 0, 70);

    /* 加载页面 */
    if (anim_type != LV_SCR_LOAD_ANIM_NONE)
    {
        lv_scr_load_anim(scr, anim_type, 500, 150, true);
        while (lv_anim_count_running() != 0) //当前页面有长文本滚动动画
        {
            lv_task_handler();
            rt_thread_mdelay(100);
        }
    }
    else
    {
        lv_scr_load(scr);
    }
}

void anniversary_gui_set_txtlabel(char *txt)
{
    lv_label_set_text(txtLabel, txt);
}

void anniversary_gui_display_date(struct tm* target)
{
    lv_label_set_text_fmt(dayLabel, "#ffa500 %02d#", anniversary_day_count);
    lv_label_set_text_fmt(targetDateLabel, "#FFFFFF %d年%d月%d日  星期%s#", target->tm_year, target->tm_mon, target->tm_mday,
                                                                            weekDayCh[target->tm_wday]);
}



