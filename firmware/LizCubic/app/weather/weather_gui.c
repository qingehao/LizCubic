#include "weather_gui.h"
#include "weather_image.h"
#include "lvgl.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app-gui.weather"
#if defined(LIZ_APP_WEATHER_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

static uint8_t is_running = 0;

LV_FONT_DECLARE(lv_font_ibmplex_115);
LV_FONT_DECLARE(lv_font_ibmplex_64);
// LV_FONT_DECLARE(msyhbd_20);
LV_FONT_DECLARE(msyhbd_22);
// LV_FONT_DECLARE(sthupo_24);
// LV_FONT_DECLARE(consolab_24);
// LV_FONT_DECLARE(yahei_consola_22);
// LV_FONT_DECLARE(jb_monob_22);
// LV_FONT_DECLARE(jb_monob_yahei_22);

static lv_style_t default_style;
static lv_style_t chFont_style;
static lv_style_t numberSmall_style;
static lv_style_t numberBig_style;
static lv_style_t btn_style;
static lv_style_t bar_style;

static lv_obj_t *scr = NULL;

static lv_obj_t *weatherImg = NULL;
static lv_obj_t *cityLabel = NULL;
static lv_obj_t *btn = NULL, *btnLabel = NULL;
static lv_obj_t *txtLabel = NULL;
static lv_obj_t *clockLabel_1 = NULL, *clockLabel_2 = NULL;
static lv_obj_t *dateLabel = NULL;
static lv_obj_t *tempImg = NULL, *tempBar = NULL, *tempLabel = NULL;
static lv_obj_t *humiImg = NULL, *humiBar = NULL, *humiLabel = NULL;
static lv_obj_t *spaceImg = NULL;

lv_timer_t *weather_update_timer = NULL;
static void weather_update_cb(lv_timer_t *timer);

// 天气图标路径的映射关系
const void *weaImage_map[] = {&weather_0, &weather_9, &weather_14, &weather_5, &weather_25,
                              &weather_30, &weather_26, &weather_11, &weather_23
                             };
// 太空人图标路径的映射关系
const void *manImage_map[] = {&man_0, &man_1, &man_2, &man_3, &man_4, &man_5, &man_6, &man_7, &man_8, &man_9};
static const char weekDayCh[7][4] = {"日", "一", "二", "三", "四", "五", "六"};
static const char airQualityCh[6][10] = {"优", "良", "轻度", "中度", "重度", "严重"};

extern uint8_t info_update_bitmap;
extern struct Weather weaInfo;
extern struct TimeStr timeInfo;
extern struct TemHumi thInfo;

static lv_font_t *msyhbd_22_ex = NULL;

void weather_gui_init(void)
{   
    if(msyhbd_22_ex == NULL)
        msyhbd_22_ex = lv_font_load("S:/fonts/msyhbd_22_ex.bin");
    lv_style_reset(&default_style);
    lv_style_init(&default_style);
    lv_style_set_bg_opa(&default_style, LV_OPA_COVER);
    lv_style_set_bg_color(&default_style,  lv_color_black());

    lv_style_reset(&chFont_style);
    lv_style_init(&chFont_style);
    lv_style_set_text_opa(&chFont_style, LV_OPA_COVER);
    lv_style_set_text_color(&chFont_style, lv_color_white());
    lv_style_set_text_font(&chFont_style, &msyhbd_22);

    lv_style_reset(&numberSmall_style);
    lv_style_init(&numberSmall_style);
    lv_style_set_text_opa(&numberSmall_style, LV_OPA_COVER);
    lv_style_set_text_color(&numberSmall_style, lv_color_white());
    lv_style_set_text_font(&numberSmall_style, &lv_font_ibmplex_64);

    lv_style_reset(&numberBig_style);
    lv_style_init(&numberBig_style);
    lv_style_set_text_opa(&numberBig_style, LV_OPA_COVER);
    lv_style_set_text_color(&numberBig_style, lv_color_white());
    lv_style_set_text_font(&numberBig_style, &lv_font_ibmplex_115);

    lv_style_reset(&btn_style);
    lv_style_init(&btn_style);
    lv_style_set_border_width(&btn_style,  0);

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

void weather_gui_load(lv_scr_load_anim_t anim_type)
{
    is_running = 0;
    
    scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &default_style, 0);
    weatherImg = lv_img_create(scr); //创建天气图标
    lv_img_set_src(weatherImg, weaImage_map[weaInfo.weather_code]);

    /* 创建城市标签 */
    cityLabel = lv_label_create(scr);
    lv_obj_add_style(cityLabel, &chFont_style, LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cityLabel, msyhbd_22_ex, 0); // 重新指定字体 到flash的字体中
    lv_label_set_recolor(cityLabel, true);
    lv_label_set_text(cityLabel, weaInfo.cityname);

    /* 创建空气质量btn */
    btn = lv_btn_create(scr);
    lv_obj_add_style(btn, &btn_style, LV_PART_MAIN);
    lv_obj_set_pos(btn, 75, 15);
    lv_obj_set_size(btn, 50, 26);
    lv_obj_set_style_bg_color(btn, lv_color_make(0xFF, 0xA5, 0x00), LV_STATE_DEFAULT | LV_PART_MAIN);

    /* 创建空气质量标签 */
    btnLabel = lv_label_create(btn);
    lv_obj_add_style(btnLabel, &chFont_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(btnLabel, airQualityCh[weaInfo.airQulity]);

    /* 创建天气信息长文本 */
    txtLabel = lv_label_create(scr);
    lv_obj_add_style(txtLabel, &chFont_style, 0);
    // LV_LABEL_LONG_SROLL_CIRC 模式一旦设置 宽度恒定等于当前文本的长度，所以下面先设置以下长度
    lv_obj_set_width(txtLabel, 130);
    lv_obj_set_style_text_font(txtLabel, msyhbd_22_ex, 0); // 重新指定字体 到flash的字体中
    lv_label_set_long_mode(txtLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    if (weaInfo.air_tips)
    {
        lv_label_set_text_fmt(txtLabel, "今日最低气温%d°C, 最高气温%d°C, %s%s.   %s",
                              weaInfo.minTemp, weaInfo.maxTmep, weaInfo.windDir, weaInfo.windLevel, \
                              weaInfo.air_tips);
    }
    else
    {
        lv_label_set_text_fmt(txtLabel, "今日最低气温%d°C, 最高气温%d°C, %s%s.   ",
                              weaInfo.minTemp, weaInfo.maxTmep, weaInfo.windDir, weaInfo.windLevel);
    }

    /* 创建时间信息 小时 分钟 */
    clockLabel_1 = lv_label_create(scr);
    lv_obj_add_style(clockLabel_1, &numberBig_style, 0);
    lv_label_set_recolor(clockLabel_1, true);
    lv_label_set_text_fmt(clockLabel_1, "%02d#ffa500 %02d#", timeInfo.hour, timeInfo.minute);
    /* 创建时间信息 秒*/
    clockLabel_2 = lv_label_create(scr);
    lv_obj_add_style(clockLabel_2, &numberSmall_style, 0);
    lv_label_set_recolor(clockLabel_2, true);
    lv_label_set_text_fmt(clockLabel_2, "%02d", timeInfo.second);
    /* 创建日期label*/
    dateLabel = lv_label_create(scr);
    lv_obj_add_style(dateLabel, &chFont_style, 0);
    lv_label_set_text_fmt(dateLabel, "%2d月%2d日  周%s", timeInfo.month, timeInfo.day,
                          weekDayCh[timeInfo.weekday]);
    /* 创建温度img*/
    tempImg = lv_img_create(scr);
    lv_img_set_src(tempImg, &temp);
    lv_img_set_zoom(tempImg, 180);

    /* 创建温度bar*/
    tempBar = lv_bar_create(scr);
    lv_obj_add_style(tempBar, &bar_style, 0);
    lv_bar_set_range(tempBar, -40, 50);   // 设置进度条表示的温度为-20~50
    lv_obj_set_size(tempBar, 60, 12);
    lv_obj_set_style_bg_color(tempBar, lv_color_make(0xFF, 0x00, 0x00), LV_STATE_DEFAULT | LV_PART_INDICATOR);
    lv_bar_set_value(tempBar, weaInfo.curTemp, LV_ANIM_OFF);
    /* 创建温度label*/
    tempLabel = lv_label_create(scr);
    lv_obj_add_style(tempLabel, &chFont_style, 0);
    lv_label_set_text_fmt(tempLabel, "%2d°C", weaInfo.curTemp);

    /* 创建湿度img*/
    humiImg = lv_img_create(scr);
    lv_img_set_src(humiImg, &humi);
    lv_img_set_zoom(humiImg, 180);
    /* 创建湿度bar*/
    humiBar = lv_bar_create(scr);
    lv_obj_add_style(humiBar, &bar_style, 0);
    lv_bar_set_range(humiBar, 0, 100);
    lv_obj_set_size(humiBar, 60, 12);
    lv_obj_set_style_bg_color(humiBar, lv_color_make(0x00, 0x00, 0xFF), LV_STATE_DEFAULT | LV_PART_INDICATOR);
    lv_bar_set_value(humiBar, weaInfo.humidity, LV_ANIM_OFF);
    /* 创建湿度label*/
    humiLabel = lv_label_create(scr);
    lv_obj_add_style(humiLabel, &chFont_style, 0);
    lv_label_set_text_fmt(humiLabel, "%2d%%", weaInfo.humidity);

    spaceImg = lv_img_create(scr);
    lv_img_set_src(spaceImg, manImage_map[0]);

    lv_obj_align(scr, LV_ALIGN_CENTER, 0, -10);
    lv_obj_align(btnLabel, LV_ALIGN_CENTER, 0, -1);
    lv_obj_align(weatherImg, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_align(cityLabel, LV_ALIGN_TOP_LEFT, 20, 15);
    lv_obj_align(txtLabel, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_align(tempImg, LV_ALIGN_LEFT_MID, 10, 70);
    lv_obj_align(tempBar, LV_ALIGN_LEFT_MID, 35, 70);
    lv_obj_align(tempLabel, LV_ALIGN_LEFT_MID, 103, 70);
    lv_obj_align(humiImg, LV_ALIGN_LEFT_MID, 0, 100);
    lv_obj_align(humiBar, LV_ALIGN_LEFT_MID, 35, 100);
    lv_obj_align(humiLabel, LV_ALIGN_LEFT_MID, 103, 100);
    lv_obj_align(spaceImg,  LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_align(clockLabel_1, LV_ALIGN_LEFT_MID, 0, 10);
    lv_obj_align(clockLabel_2, LV_ALIGN_LEFT_MID, 165, 9);
    lv_obj_align(dateLabel, LV_ALIGN_LEFT_MID, 10, 32);

    if (anim_type != LV_SCR_LOAD_ANIM_NONE)
    {
        if (anim_type == LV_SCR_LOAD_ANIM_FADE_ON)
        {
            lv_scr_load_anim(scr, anim_type, 200, 50, true);
        }
        else
        {
            lv_scr_load_anim(scr, anim_type, 500, 150, true);
        }
        while (lv_anim_count_running() != 1) //当前页面有长文本滚动动画
        {
            lv_task_handler();
            rt_thread_mdelay(100);
        }
    }
    else
    {
        lv_scr_load(scr);
    }

    weather_update_timer = lv_timer_create(weather_update_cb, 50, NULL);
    lv_timer_set_repeat_count(weather_update_timer, -1);
    lv_timer_ready(weather_update_timer);
    is_running = 1;
}

static void weather_update_cb(lv_timer_t *timer)
{
    if (is_running)
    {
        if (info_update_bitmap & UPDATE_MASK_TIME)
        {
            display_time(timeInfo, LV_SCR_LOAD_ANIM_NONE);  //刷新屏幕时间显示
            info_update_bitmap &= ~UPDATE_MASK_TIME;
        }
        // if (info_update_bitmap & UPDATE_MASK_TH)
        // {
        //     display_th(thInfo, LV_SCR_LOAD_ANIM_NONE);  //刷新屏幕温湿度显示
        //     info_update_bitmap &= ~UPDATE_MASK_TH;
        // }
        if (info_update_bitmap & UPDATE_MASK_WEA)
        {
            display_weather(weaInfo, LV_SCR_LOAD_ANIM_NONE); //刷新屏幕天气显示
            info_update_bitmap &= ~UPDATE_MASK_WEA;
        }
        display_space();
    }
}

void display_weather(struct Weather weaInfo, lv_scr_load_anim_t anim_type)
{
    lv_label_set_text(cityLabel, weaInfo.cityname);
    lv_label_set_text(btnLabel, airQualityCh[weaInfo.airQulity]);
    lv_img_set_src(weatherImg, weaImage_map[weaInfo.weather_code]);
    // 今天最高气温12°C，最低气温-1°C，西北风3级。儿童、老年人及心脏病、呼吸系统疾病患者应尽量减少体力消耗大的户外活动。
    if (weaInfo.air_tips)
    {
        lv_label_set_text_fmt(txtLabel, "今日最低气温%d°C, 最高气温%d°C, %s%s.   %s",
                              weaInfo.minTemp, weaInfo.maxTmep, weaInfo.windDir, weaInfo.windLevel, \
                              weaInfo.air_tips);
    }
    else
    {
        lv_label_set_text_fmt(txtLabel, "今日最低气温%d°C, 最高气温%d°C, %s%s.   ",
                              weaInfo.minTemp, weaInfo.maxTmep, weaInfo.windDir, weaInfo.windLevel);
    }

    lv_bar_set_value(tempBar, weaInfo.curTemp, LV_ANIM_OFF);
    lv_label_set_text_fmt(tempLabel, "%2d°C", weaInfo.curTemp);
    lv_bar_set_value(humiBar, weaInfo.humidity, LV_ANIM_OFF);
    lv_label_set_text_fmt(humiLabel, "%2d%%", weaInfo.humidity);

}

void display_time(struct TimeStr timeInfo, lv_scr_load_anim_t anim_type)
{

    lv_label_set_text_fmt(clockLabel_1, "%02d#ffa500 %02d#", timeInfo.hour, timeInfo.minute);
    lv_label_set_text_fmt(clockLabel_2, "%02d", timeInfo.second);
    lv_label_set_text_fmt(dateLabel, "%2d月%2d日  周%s", timeInfo.month, timeInfo.day,
                          weekDayCh[timeInfo.weekday]);
}

// void display_th(struct TemHumi thInfo, lv_scr_load_anim_t anim_type)
// {
//     lv_bar_set_value(tempBar, thInfo.temperature, LV_ANIM_OFF);
//     lv_label_set_text_fmt(tempLabel, "%2d°C", thInfo.temperature);
//     lv_bar_set_value(humiBar, thInfo.humidity, LV_ANIM_OFF);
//     lv_label_set_text_fmt(humiLabel, "%d%%", thInfo.humidity);
// }

void display_space(void)
{
    static int _spaceIndex = 0;
    if (NULL != scr && lv_scr_act() == scr)
    {
        lv_img_set_src(spaceImg, manImage_map[_spaceIndex]);
        _spaceIndex = (_spaceIndex + 1) % 10;
    }
}

void weather_gui_unload(void)
{
    is_running = 0;
    lv_timer_del(weather_update_timer);
    scr = NULL;
}



/*
    根据天气现象获得相应代码
    else if没用, 但我觉得好看
*/
int get_weather_code(char *wea)
{
    if (rt_strcmp("qing", wea) == 0) return 0;

    else if (rt_strcmp("yin", wea) == 0) return 1;

    else if (rt_strcmp("yu", wea) == 0) return 2;

    else if (rt_strcmp("yun", wea) == 0) return 3;

    else if (rt_strcmp("bingbao", wea) == 0) return 4;

    else if (rt_strcmp("wu", wea) == 0) return 5;

    else if (rt_strcmp("shachen", wea) == 0) return 6;

    else if (rt_strcmp("lei", wea) == 0) return 7;

    else if (rt_strcmp("xue", wea) == 0) return 8;

    return 0;
}
int get_airQulity_level(int q)
{
    if (q < 50) return 0;

    else if (q < 100) return 1;

    else if (q < 150) return 2;

    else if (q < 200) return 3;

    else if (q < 300) return 4;

    return 5;
}

