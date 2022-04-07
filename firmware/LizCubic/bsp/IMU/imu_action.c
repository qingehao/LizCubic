#include "imu_action.h"
#include "rtthread.h"
#include <rtthread.h>
#include <finsh.h>
#include "MD_Ported_to_RTT.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "invensense.h"
#include "invensense_adv.h"
#include "eMPL_outputs.h"
#include "mltypes.h"
#include "mpu.h"
#include "log.h"
#include "packet.h"
#include "liz_config.h"
#define DBG_TAG    "liz.bsp.imu"
#if defined(LIZ_BSP_IMU_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>


#define RT_MPU_DEVICE_NAME "i2c2"
//q30，q16格式,long转float时的除数.
#define q30  1073741824.0f
#define q16  65536.0f
struct _imu
{
    float pitch;
    float roll;
    float yaw;
    float pitch_offset;
    float roll_offset;
    float yaw_offset;
};
extern struct rt_mpu_device *mpu_dev;
extern int mpu_dev_init_flag; /* Flag to show if the mpu device is inited. */

unsigned char *mpl_key = (unsigned char *)"eMPL 5.1";
struct _imu imu = {0};
static struct rt_timer imu_update_timer;
static rt_sem_t imu_data_sem = RT_NULL;

static uint8_t run_self_test(void);
static uint8_t mpu_mpl_get_data(float *pitch, float *roll, float *yaw);
static void imu_get_offset();
//陀螺仪方向设置
static signed char gyro_orientation[9] = { 1, 0, 0,
                                           0, 1, 0,
                                           0, 0, 1
                                         };

static void imu_data_update_cb(void *parameter)
{
    rt_sem_release(imu_data_sem);
}
static void imu_data_update(void *arg)
{
    float pitch, roll, yaw;
    while (1)
    {
        rt_sem_take(imu_data_sem, RT_WAITING_FOREVER);
        while (mpu_mpl_get_data(&pitch, &roll, &yaw))
        {
            rt_thread_mdelay(1);
        }
        imu.pitch = pitch;
        imu.roll = roll;
        imu.yaw = yaw;
    }
}

/**
 * @brief imu dmp初始化
 * @param progress 进度
 * @return 返回值:0,成功 其他,错误代码
 */
int mpu_dmp_init(void)
{
    int ret = 0;
    struct int_param_s int_param;
    uint8_t accel_fsr = 0;
    uint16_t gyro_rate = 0, gyro_fsr = 0;
    if (mpu_dev_init_flag == 0)
    {
        mpu_dev = rt_mpu_init(RT_MPU_DEVICE_NAME, RT_NULL);
        if (!mpu_dev) return -1;
        else mpu_dev_init_flag = 1;
    }
    ret = mpu_init(&int_param);
    LOG_D("mpu_init end");
    if (ret) return 1;
    else
    {
        LOG_D("inv_init_mpl..");
        ret = inv_init_mpl();     //初始化MPL
        if (ret) return 2;
        inv_enable_quaternion();
        inv_enable_9x_sensor_fusion();
        inv_enable_fast_nomot();
        inv_enable_gyro_tc();
        inv_enable_eMPL_outputs();
        LOG_D("inv_start_mpl..");
        ret = inv_start_mpl();    //开启MPL
        if (ret) return 3;
        LOG_D("mpu_set_sensors..");
        ret = mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL); //设置所需要的传感器
        rt_thread_mdelay(3);
        if (ret) return 4;
        LOG_D("mpu_configure_fifo..");
        ret = mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);   //设置FIFO
        if (ret) return 5;
        LOG_D("mpu_set_sample_rate..");
        ret = mpu_set_sample_rate(DEFAULT_MPU_HZ);              //设置采样率
        if (ret) return 6;
        mpu_get_sample_rate(&gyro_rate);
        mpu_get_gyro_fsr(&gyro_fsr);
        mpu_get_accel_fsr(&accel_fsr);
        inv_set_gyro_sample_rate(1000000L / gyro_rate);
        inv_set_accel_sample_rate(1000000L / gyro_rate);
        inv_set_gyro_orientation_and_scale(
            inv_orientation_matrix_to_scalar(gyro_orientation), (long)gyro_fsr << 15);
        inv_set_accel_orientation_and_scale(
            inv_orientation_matrix_to_scalar(gyro_orientation), (long)accel_fsr << 15);
        LOG_D("dmp_load_motion_driver_firmware..");
        ret = dmp_load_motion_driver_firmware();                     //加载dmp固件
        if (ret) return 7;
        LOG_D("dmp_set_orientation..");
        ret = dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));//设置陀螺仪方向
        if (ret) return 8;
        LOG_D("dmp_enable_feature..");
        ret = dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |             //设置dmp功能
                                 DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
                                 DMP_FEATURE_GYRO_CAL);
        if (ret) return 9;
        LOG_D("dmp_set_fifo_rate..");
        ret = dmp_set_fifo_rate(DEFAULT_MPU_HZ);    //设置DMP输出速率(最大不超过200Hz)
        if (ret) return 10;
        LOG_D("run_self_test..");
        ret = run_self_test();      //自检
        if (ret) return 11;
        LOG_D("mpu_set_dmp_state..");
        ret = mpu_set_dmp_state(1); //使能DMP
        if (ret) return 12;
        // imu_get_offset();
    }
    imu_data_sem = rt_sem_create("imu", 0, RT_IPC_FLAG_FIFO);
    rt_timer_init(&imu_update_timer, "imu_update",
                  imu_data_update_cb,
                  RT_NULL,
                  10,
                  RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(&imu_update_timer);
    rt_thread_t tid = rt_thread_create("imu_data_update", imu_data_update, RT_NULL,
                                       BSP_IMU_DATA_UPDATE_THREAD_STACK_SIZE,
                                       BSP_IMU_DATA_UPDATE_THREAD_PRIO, 10);
    if (tid != RT_NULL) rt_thread_startup(tid);
    else return 13;
    mpu_reset_fifo();
    return 0;
}
static void imu_get_offset()
{
    float pitch, roll, yaw;
    while (mpu_mpl_get_data(&pitch, &roll, &yaw))
    {
        rt_thread_mdelay(10);
    }
    imu.pitch_offset = pitch;
    imu.roll_offset = roll;
    imu.yaw_offset = yaw;
}
/**
* @brief imu获得运动方向
* @param struct imu_config cfg imu参数
* @return enum MOV_DIR 运动方向
*/
enum MOV_DIR imu_get_move_dir()
{
    enum MOV_DIR dir_now = DIR_UNKNOWN;
    // imu.pitch -= imu.pitch_offset;
    // imu.roll -= imu.roll_offset;
    // imu.yaw -= imu.yaw_offset;
    // char buf[40]={0};
    // sprintf(buf,"imu.roll:%.2f\nimu.pitch:%.2f\n",imu.roll,imu.pitch);
    // rt_kprintf(buf);
    //判断当前方向
    if (imu.pitch  > IMU_RIGHT_THRESHOLD) dir_now = DIR_RIGHT;
    else if (imu.pitch < IMU_LEFT_THRESHOLD) dir_now = DIR_LEFT;
    else if ( imu.roll < IMU_UP_THRESHOLD && imu.roll > 0 ) dir_now = DIR_UP;
    else if (imu.roll > IMU_DOWN_THRESHOLD && imu.roll < 0) dir_now = DIR_DOWN;
    else dir_now = DIR_NOMOV;
    return dir_now;
}
/**
 * @brief MPU6050自测试
 *
 * @return uint8_t 0,正常 其他,失败
 */
static uint8_t run_self_test(void)
{
    int result;
    //char test_packet[4] = {0};
    long gyro[3], accel[3];
    result = mpu_run_self_test(gyro, accel);
    if (result == 0x7)
    {
        /* Test passed. We can trust the gyro data here, so let's push it down
        * to the DMP.
        */
        unsigned short accel_sens;
        float gyro_sens;
        mpu_get_gyro_sens(&gyro_sens);
        gyro[0] = (long)(gyro[0] * gyro_sens);
        gyro[1] = (long)(gyro[1] * gyro_sens);
        gyro[2] = (long)(gyro[2] * gyro_sens);
        //inv_set_gyro_bias(gyro, 3);
        dmp_set_gyro_bias(gyro);
        mpu_get_accel_sens(&accel_sens);
        accel[0] *= accel_sens;
        accel[1] *= accel_sens;
        accel[2] *= accel_sens;
        // inv_set_accel_bias(accel, 3);
        dmp_set_accel_bias(accel);
        return 0;
    }
    else return 1;
}

/**
 * @brief 得到dmp处理后的数据(注意,本函数需要比较多堆栈,局部变量有点多)
 *
 * @param pitch 俯仰角 精度:0.1°   范围:-90.0° <---> +90.0°
 * @param roll 横滚角  精度:0.1°   范围:-180.0°<---> +180.0°
 * @param yaw 精度:0.1°   范围:-180.0°<---> +180.0°
 * @return uint8_t 0,正常 其他,失败
 */
static uint8_t mpu_dmp_get_data(float *pitch, float *roll, float *yaw)
{
    float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
    unsigned long sensor_timestamp;
    short gyro[3], accel[3], sensors;
    unsigned char more;
    long quat[4];
    if (dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors, &more))return 1;
    /* Gyro and accel data are written to the FIFO by the DMP in chip frame and hardware units.
     * This behavior is convenient because it keeps the gyro and accel outputs of dmp_read_fifo and mpu_read_fifo consistent.
    **/
    /*if (sensors & INV_XYZ_GYRO )
    send_packet(PACKET_TYPE_GYRO, gyro);
    if (sensors & INV_XYZ_ACCEL)
    send_packet(PACKET_TYPE_ACCEL, accel); */
    /* Unlike gyro and accel, quaternions are written to the FIFO in the body frame, q30.
     * The orientation is set by the scalar passed to dmp_set_orientation during initialization.
    **/
    if (sensors & INV_WXYZ_QUAT)
    {
        q0 = quat[0] / q30; //q30格式转换为浮点数
        q1 = quat[1] / q30;
        q2 = quat[2] / q30;
        q3 = quat[3] / q30;
        //计算得到俯仰角/横滚角/航向角
        *pitch = asin(-2 * q1 * q3 + 2 * q0 * q2) * 57.3; // pitch
        *roll  = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2 * q2 + 1) * 57.3; // roll
        *yaw   = atan2(2 * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * 57.3; //yaw
    }
    else return 2;
    return 0;
}
/**
 * @brief 得到mpl处理后的数据(注意,本函数需要比较多堆栈,局部变量有点多)
 *
 * @param pitch pitch:俯仰角 精度:0.1°   范围:-90.0° <---> +90.0°
 * @param roll roll:横滚角  精度:0.1°   范围:-180.0°<---> +180.0°
 * @param yaw yaw:航向角   精度:0.1°   范围:-180.0°<---> +180.0°
 * @return uint8_t 0,正常 其他,失败
 */
static uint8_t mpu_mpl_get_data(float *pitch, float *roll, float *yaw)
{
    unsigned long sensor_timestamp, timestamp;
    short gyro[3], accel_short[3], compass_short[3], sensors;
    unsigned char more;
    long compass[3], accel[3], quat[4], temperature;
    long data[9];
    int8_t accuracy;

    if (dmp_read_fifo(gyro, accel_short, quat, &sensor_timestamp, &sensors, &more))return 1;

    if (sensors & INV_XYZ_GYRO)
    {
        inv_build_gyro(gyro, sensor_timestamp);         //把新数据发送给MPL
        mpu_get_temperature(&temperature, &sensor_timestamp);
        inv_build_temp(temperature, sensor_timestamp);  //把温度值发给MPL，只有陀螺仪需要温度值
    }

    if (sensors & INV_XYZ_ACCEL)
    {
        accel[0] = (long)accel_short[0];
        accel[1] = (long)accel_short[1];
        accel[2] = (long)accel_short[2];
        inv_build_accel(accel, 0, sensor_timestamp);    //把加速度值发给MPL
    }
    inv_execute_on_data();
    inv_get_sensor_type_euler(data, &accuracy, &timestamp);

    *roll  = (data[0] / q16);
    *pitch = -(data[1] / q16);
    *yaw   = -data[2] / q16;
    return 0;
}
