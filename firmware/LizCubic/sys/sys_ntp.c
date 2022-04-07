#include "at_device_sim76xx.h"
#include "network.h"
#include <rtthread.h>
#include "liz_config.h"
#define DBG_TAG    "liz.sys.ntp"
#if defined(LIZ_NET_MQTT_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

static struct rt_work ntp_sync_work;

static void ntp_sync_work_func(struct rt_work *work, void *work_data)
{
    if (check_network())
    {
        if (sim76xx_ntp_sync() != 0)
        {
            rt_work_submit(work, rt_tick_from_millisecond(1000));
        }
        else
        {
            rt_work_submit(work, rt_tick_from_millisecond(NTP_AUTO_SYNC_PERIOD * 1000));
        }

    }
    else
    {
        rt_work_submit(work, rt_tick_from_millisecond(1 * 1000));
    }
}

int ntp_auto_sync_init(void)
{
    rt_work_init(&ntp_sync_work, ntp_sync_work_func, RT_NULL);
    rt_work_submit(&ntp_sync_work, rt_tick_from_millisecond(NTP_AUTO_SYNC_PERIOD * 1000));
    return RT_EOK;
}

#ifdef FINSH_USING_MSH
#include <finsh.h>
static void cmd_ntp_sync(int argc, char **argv)
{
    sim76xx_ntp_sync();
}
MSH_CMD_EXPORT_ALIAS(cmd_ntp_sync, ntp_sync, Update time by NTP(Network Time Protocol): ntp_sync [host_name]);
#endif /* RT_USING_FINSH */
