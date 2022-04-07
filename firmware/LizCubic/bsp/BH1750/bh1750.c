#include "rtthread.h"
#include "board.h"
#include "bh1750.h"

#include "liz_config.h"
#define DBG_TAG    "liz.bsp.bh1750"
#if defined(LIZ_BSP_BH1750_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

static struct rt_i2c_bus_device *bh1750_dev = RT_NULL;
#define BH1750_DEV_NAME "i2c1"

static int bh1750_read_regs(rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = BH1750_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    if (1 == rt_i2c_transfer(bh1750_dev, &msgs, 1))
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static int bh1750_write_cmd(rt_uint8_t cmd)
{
    struct rt_i2c_msg msgs;

    msgs.addr = BH1750_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = &cmd;
    msgs.len = 1;

    if (1 == rt_i2c_transfer(bh1750_dev, &msgs, 1))
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static int bh1750_set_measure_mode(rt_uint8_t mode, rt_uint8_t m_time)
{
    RT_ASSERT(bh1750_dev);

    if (RT_EOK == bh1750_write_cmd(BH1750_RESET) &&
        RT_EOK == bh1750_write_cmd(mode))
    {
        rt_thread_mdelay(m_time);
        return RT_EOK;
    }
    else
    {
        LOG_E("bh1750 set measure mode failed!");
        return -RT_ERROR;
    }
}

int bh1750_power_on()
{
    RT_ASSERT(bh1750_dev);

    if (RT_EOK == bh1750_write_cmd( BH1750_POWER_ON))
    {
        return RT_EOK;
    }
    else
    {
        LOG_E("bh1750 power on failed!");
        return -RT_ERROR;
    }
}

int bh1750_power_down()
{
    RT_ASSERT(bh1750_dev);

    if (RT_EOK == bh1750_write_cmd(BH1750_POWER_DOWN))
    {
        return RT_EOK;
    }
    else
    {
        LOG_D("bh1750 power down failed!");
        return -RT_ERROR;
    }
}

int bh1750_init()
{
    bh1750_dev = rt_i2c_bus_device_find(BH1750_DEV_NAME);
    if (bh1750_dev == RT_NULL)
    {
        LOG_E("Can't find bh1750 device on '%s' ", BH1750_DEV_NAME);
        return -RT_ERROR;
    }
    return RT_EOK;
}

float bh1750_read_light()
{
    rt_uint8_t temp[2];
    float current_light = 0;
    if(bh1750_dev)
    {
        bh1750_set_measure_mode(BH1750_CON_H_RES_MODE2, 120);
        bh1750_read_regs(2, temp);
        current_light = ((float)((temp[0] << 8) + temp[1]) / 1.2);
        return current_light;
    }
    return -1;
}

