#ifndef _IMU_ACTION_H
#define _IMU_ACTION_H

#include "stdint.h"
enum MOV_DIR
{
    DIR_NOMOV= 0,
    DIR_RIGHT,
    DIR_LEFT,
    DIR_UP,
    DIR_DOWN,
    DIR_UNKNOWN
};
enum IMU_ACTION_TYPE
{
    MOVE_RIGHT = 0,
    MOVE_RIGHT_LONG,
    MOVE_LEFT,
    MOVE_LEFT_LONG,
    MOVE_UP,
    MOVE_UP_LONG,
    MOVE_DOWN,
    MOVE_DONW_LONG,
    SHAKE,
    UNKNOWN
};
struct _app_action_info
{
    enum IMU_ACTION_TYPE act_type;
    uint8_t is_valid; //是否有效
    int duration; //持续时间
    uint8_t inter; //指向中断应用
};

int mpu_dmp_init(void);
enum MOV_DIR imu_get_move_dir();

#endif
