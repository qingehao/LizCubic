#ifndef APP_WEATHER_GUI_H
#define APP_WEATHER_GUI_H


/* 信息更新标志位 */
#define UPDATE_MASK_TIME (1<<0)
#define UPDATE_MASK_TH   (1<<1)
#define UPDATE_MASK_WEA  (1<<2)

struct Weather
{
    int weather_code; // 天气现象代码
    int curTemp;      // 当前温度
    int maxTmep;      // 最高气温
    int minTemp;      // 最低气温
    char windDir[10]; // 风向
    char windLevel[10]; // 风力等级
    int humidity; //湿度
    char cityname[10]; // 城市名
    int airQulity;
    int air_pm25; // pm2.5指数
    char *air_tips;
};
struct TemHumi
{
    int temperature;  // 温度
    int humidity;     // 湿度
};
struct TimeStr
{
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int weekday;
};

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
    void weather_gui_init(void);
    void weather_gui_load(lv_scr_load_anim_t anim_type);
    void weather_gui_unload(void);
    void display_weather(struct Weather weaInfo, lv_scr_load_anim_t anim_type);
    void display_time(struct TimeStr timeInfo, lv_scr_load_anim_t anim_type);
    // void display_th(struct TemHumi thInfo, lv_scr_load_anim_t anim_type);
    void display_space(void);
    int get_airQulity_level(int q);
    int get_weather_code(char *wea);
#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
    extern const lv_img_dsc_t app_weather;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif