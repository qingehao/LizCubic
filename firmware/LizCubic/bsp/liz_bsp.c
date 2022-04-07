#include "rtdevice.h"
#include "rtthread.h"
#include "ws2812b.h"
#include "imu_action.h"
#include "bh1750.h"
#include "kvdb.h"
#include "liz_bsp.h"
#include "liz_config.h"

#define DBG_TAG    "liz.bsp"
#if defined(LIZ_BSP_DEBUG)
#define DBG_LVL    DBG_LOG
#else 
#define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

// sht20_device_t liz_th;
ws2812_rgbled_t liz_rgbled;
struct _liz_bsp liz_bsp;

static enum IMU_ACTION_TYPE liz_bsp_get_act_info();
static float liz_bsp_get_temperature();
static float liz_bsp_get_humidity();

int liz_bsp_init()
{
    int ret =0 ;
    kvdb_init();
    liz_bsp.db_set_data = kvdb_set_data;
    liz_bsp.db_get_data = kvdb_get_data;
    liz_bsp.db_set_blob = kvdb_set_blob;
    liz_bsp.db_get_blob = kvdb_get_blob;

    liz_bsp.rgbled = ws2812b_rgb_led_init(1);

    ret = bh1750_init();
    if(ret == RT_EOK)
    {
        liz_bsp.get_ambient_light = bh1750_read_light;
    }
    else
    {
        liz_bsp.get_ambient_light = RT_NULL;
    }

    ret = mpu_dmp_init();
    while(ret!=0)
    {
        LOG_E("mpu dmp init fail (%d)",ret);
        ret = mpu_dmp_init();
        rt_thread_mdelay(100);
    }
    liz_bsp.get_act_info=liz_bsp_get_act_info;
    return ret;
}

static enum IMU_ACTION_TYPE liz_bsp_get_act_info()
{
    enum IMU_ACTION_TYPE cur_act_info = UNKNOWN; 
    enum MOV_DIR dir_now = DIR_UNKNOWN;
    dir_now = imu_get_move_dir();
    switch(dir_now)
    {
        case DIR_NOMOV: cur_act_info = UNKNOWN; break;
        case DIR_RIGHT: cur_act_info = MOVE_RIGHT;  break;
        case DIR_LEFT: cur_act_info = MOVE_LEFT; break;
        case DIR_UP: cur_act_info = MOVE_UP; break;
        case DIR_DOWN: cur_act_info = MOVE_DOWN; break;
        case DIR_UNKNOWN: cur_act_info = UNKNOWN; break;
    } 
    return cur_act_info;     
}

// static float liz_bsp_get_temperature()
// {
//     return sht20_read_temperature(liz_th);
// }

// static float liz_bsp_get_humidity()
// {
//     return sht20_read_humidity(liz_th);
// }



