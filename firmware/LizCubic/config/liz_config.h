#ifndef _LIZ_CFG_H_
#define _LIZ_CFG_H_
/* -------IMU------- */
/**
 * 向左倒 pitch减小 -10
 * 向右倒 pitch增大 15
 * 向后倒 DOWN roll -160 -> -130
 * 向前倒 UP roll -160 -> 180 -> 170
 */
#define IMU_LEFT_THRESHOLD -16.0f
#define IMU_RIGHT_THRESHOLD 16.0f
#define IMU_DOWN_THRESHOLD -140.0f
#define IMU_UP_THRESHOLD 178.0f  //

/* -------APP WEATHER------- */
/* 信息更新时间间隔 单位 100ms */
#define TIME_UPDATE_INVT (5)
#define TH_UPDATE_INVT  (200)
#define WEA_UPDATE_INVT (30*60*10)
/* 一刻天气API */
#define WEATHER_API_APPID      "XXXXXXXX"
#define WEATHER_API_APPSECRET  "XXXXXXXX"
#define WEA_AIR_TIPS_SIZE (256)

#define APP_WEATHER_INFO_UPDATE_THREAD_STACK_SIZE  1024
#define APP_WEATHER_INFO_UPDATE_THREAD_PRIO 19

/* -------BSP IMU------- */
#define BSP_IMU_DATA_UPDATE_THREAD_STACK_SIZE 768 
#define BSP_IMU_DATA_UPDATE_THREAD_PRIO 18   // IMU的优先级放低
#define BSP_IMU_DATA_OUTPUT_HZ 40 // IMU的输出频率

/* -------BSP ADC_FFT------- */
#define BSP_ADC_FFT_GEN_THREAD_STACK_SIZE 768
#define BSP_ADC_FFT_GEN_THREAD_PRIO 15

/* -------network MQTT------- */
#define LIZ_MQTT_URI                "tcp://xxx.xxx.xxx.xxx:xxxx"

/* -------LIZ_APP------- */
#define LIZ_APP_THREAD_STACK_SIZE 4096
#define LIZ_APP_THREAD_PRIO 9

/* -------LVGL------- */
#define LV_THREAD_STACK_SIZE 4096
#define LV_THREAD_PRIO (11)

#define NTP_AUTO_SYNC_PERIOD 3600

#include "liz_log_cfg.h"
#include "user_dma_config.h"

#endif
