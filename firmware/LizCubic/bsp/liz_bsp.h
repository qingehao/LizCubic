#ifndef _LIZ_BSP_H_
#define _LIZ_BSP_H_
#include "imu_action.h"
#include "ws2812b.h"
#include "stdint.h"

struct _liz_bsp{

    float (*get_temperature)(void);
    float (*get_humidity)(void);
    void (*db_set_data)(char *key, char *value); 
    int (*db_get_data)(char *key, char *value);
    void (*db_set_blob)(char *key, void *value, int len);
    int (*db_get_blob)(char *key, void *value, int len);
    enum IMU_ACTION_TYPE (*get_act_info)(void);
    ws2812_rgbled_t rgbled;
    float (*get_ambient_light)(void); // 获取环境光强
};

extern struct _liz_bsp liz_bsp;

int liz_bsp_init();

#endif
