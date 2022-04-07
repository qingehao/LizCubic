#include "rtthread.h"
#include "rtdevice.h"
#include "weather_gui.h"
#include "weather.h"
#include "string.h"
#include "sys/time.h"
#include "liz_bsp.h"
#include "interface.h"
#include "network.h"
#include "jsmn.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app.weather"
#if defined(LIZ_APP_WEATHER_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

#define WEATHER_NOW_API        "http://yiketianqi.com/api?unescape=1&version=v6&appid=%s&appsecret=%s"

static rt_thread_t info_update_tid = RT_NULL;

struct Weather weaInfo = {0};
struct TimeStr timeInfo = {0};
struct TemHumi thInfo = {0};

static int time_update(struct TimeStr *p);
static int th_update(struct TemHumi *p);
static int weather_update(struct Weather *p);
static void get_weather_info_update_bitmap(void *arg);

static void info_update_thread(void *args);

uint8_t info_update_bitmap = 0;

static struct rt_work weather_sync_work;
static void weather_sync_work_func(struct rt_work *work, void *work_data);
static uint8_t is_init = 0;

static void weather_init()
{
    info_update_bitmap = 0;
    if (is_init) return;
    is_init = 1;
    strcpy(weaInfo.cityname, "上海");
    strcpy(weaInfo.windDir, "东南风");
    strcpy(weaInfo.windLevel, "3级");
    weaInfo.air_tips = rt_calloc(1, WEA_AIR_TIPS_SIZE);
    if (weaInfo.air_tips == RT_NULL)
    {
        LOG_W("no memory for air tips");
    }
    weather_gui_init();
    rt_work_init(&weather_sync_work, weather_sync_work_func, RT_NULL);
    if (info_update_tid == RT_NULL)
    {
        info_update_tid = rt_thread_create("weather", info_update_thread, RT_NULL,
                                           APP_WEATHER_INFO_UPDATE_THREAD_STACK_SIZE,
                                           APP_WEATHER_INFO_UPDATE_THREAD_PRIO, 10);
        if (info_update_tid != RT_NULL)
        {
            rt_thread_startup(info_update_tid);
        }
    }
}
static void weather_load(lv_scr_load_anim_t anim_type)
{
    static uint8_t first_load = 1;
    if (first_load)
    {
        first_load = 0;
    }
    else
    {
        if (check_network())
        {
            rt_work_submit(&weather_sync_work, rt_tick_from_millisecond(2000));
        }
    }
    weather_gui_load(anim_type);
}

static void process(struct APP_CONTROLLER *sys, struct _app_action_info *act_info)
{

}

static void exit_callback()
{
    weather_gui_unload();
}

static void info_update_thread(void *args)
{
    int time_update_cnt = 0;
    int wea_update_cnt = 0;
    int ret = 0;
    while (1)
    {
        if (time_update_cnt % TIME_UPDATE_INVT == 0) //更新时间
        {
            time_update(&timeInfo);
            info_update_bitmap |= UPDATE_MASK_TIME;
        }
        if (wea_update_cnt % WEA_UPDATE_INVT == 0) //更新天气信息
        {
            //占用时间过长 启用一个工作队列
            if (check_network())
            {
                rt_work_submit(&weather_sync_work, rt_tick_from_millisecond(2000));
            }
            else
            {
                wea_update_cnt--;
            }
        }
        time_update_cnt++;
        wea_update_cnt++;
        rt_thread_mdelay(100);
    }
    info_update_tid = RT_NULL;
}
static void get_weather_info_update_bitmap(void *arg)
{
    *(uint8_t *)arg = info_update_bitmap;
}
static int time_update(struct TimeStr *p)
{
    time_t timep;
    struct tm *p_tm;
    time(&timep);
    p_tm = localtime(&timep);
    p->month = p_tm->tm_mon + 1;
    p->day = p_tm->tm_mday;
    p->hour = p_tm->tm_hour;
    p->minute = p_tm->tm_min;
    p->second = p_tm->tm_sec;
    p->weekday = p_tm->tm_wday;
    return 0;
}

// static int th_update(struct TemHumi *p)
// {
//     p->humidity = (int)liz_bsp.get_humidity();
//     p->temperature = (int)liz_bsp.get_temperature();
//     return 0;
// }

static void weather_sync_work_func(struct rt_work *work, void *work_data)
{
    int ret = 0;
    ret = weather_update(&weaInfo);
    if (ret != RT_EOK)
    {
        rt_work_submit(&weather_sync_work, rt_tick_from_millisecond(2000)); // 2s后重新获取
    }
    else
    {
        info_update_bitmap |= UPDATE_MASK_WEA;
    }
}
#include <string.h>
static int fetch_key(const char *json_string, jsmntok_t *t, const char *str)
{
    int length;
    length = strlen(str);
    if ((t->type == JSMN_STRING) && (length == (t->end - t->start)))
    {
        return strncmp(json_string + t->start, str, length);
    }
    else
    {
        return -1;
    }
}
/*
    天气JSON数据解析
*/
static int weather_json_parser(char *buffer, struct Weather *p)
{
    int ret = 0;
    jsmn_parser p_parser;
    jsmn_init(&p_parser);
    jsmntok_t t[62] = {0};
    char str_buf[10] = {0};
    ret = jsmn_parse(&p_parser, buffer, strlen(buffer), t, sizeof(t) / sizeof(t[0]));
    if (ret < 0)
    {
        LOG_E("(weather_json_parser)failed to parse json !");
        return RT_ERROR;
    }
    for (int i = 0; i < ret; i++)
    {
        if (fetch_key(buffer, &t[i], "city") == 0)
        {
            snprintf(p->cityname, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            LOG_RAW("city: %s\n", p->cityname);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "wea_img") == 0)
        {
            snprintf(str_buf, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            p->weather_code = get_weather_code(str_buf);
            LOG_RAW("wea_img: %s\n", str_buf);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "tem") == 0)
        {
            snprintf(str_buf, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            p->curTemp = atoi(str_buf);
            LOG_RAW("tem: %d°C\n", p->curTemp);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "tem1") == 0)
        {
            snprintf(str_buf, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            p->maxTmep = atoi(str_buf);
            LOG_RAW("tem_day: %d°C\n", p->maxTmep);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "tem2") == 0)
        {
            snprintf(str_buf, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            p->minTemp = atoi(str_buf);
            LOG_RAW("tem_night: %d°C\n", p->minTemp);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "win") == 0)
        {
            snprintf(p->windDir, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            LOG_RAW("win: %s\n", p->windDir);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "win_speed") == 0)
        {
            snprintf(p->windLevel, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            LOG_RAW("win_speed: %s\n", p->windLevel);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "humidity") == 0)
        {
            snprintf(str_buf, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            p->humidity = atoi(str_buf);
            LOG_RAW("humidity: %d\n", p->humidity);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "air") == 0)
        {
            snprintf(str_buf, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            p->airQulity = get_airQulity_level(atoi(str_buf));
            LOG_RAW("air: %s\n", str_buf);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "air_pm25") == 0)
        {
            snprintf(str_buf, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
            p->air_pm25 = atoi(str_buf);
            LOG_RAW("air_pm25: %d\n", p->air_pm25);
            i++;
        }
        else if (fetch_key(buffer, &t[i], "air_tips") == 0)
        {
            if (weaInfo.air_tips)
            {
                rt_memset(weaInfo.air_tips, 0, WEA_AIR_TIPS_SIZE);
                snprintf(weaInfo.air_tips, t[i + 1].end - t[i + 1].start + 1, buffer + t[i + 1].start);
                LOG_RAW("air tips: %s\n", weaInfo.air_tips);
            }

            i++;
        }
    }
    return RT_EOK;
}
static int weather_update(struct Weather *p)
{
#define GET_HEADER_BUFSZ               1024
#define GET_RESP_BUFSZ                 2048
    struct webclient_session *session = RT_NULL;
    unsigned char *buffer = RT_NULL;
    int index, ret = 0;
    int bytes_read, resp_status;
    int content_length = -1;
    char str_buf[100] = "";
    snprintf(str_buf, sizeof(str_buf), WEATHER_NOW_API, WEATHER_API_APPID, WEATHER_API_APPSECRET);
    buffer = (unsigned char *) web_malloc(GET_RESP_BUFSZ);
    if (buffer == RT_NULL)
    {
        LOG_E("no memory for receive buffer");
        ret = -RT_ENOMEM;
        goto __exit;
    }
    /* create webclient session and set header response size */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        ret = -RT_ENOMEM;
        goto __exit;
    }
    /* send GET request by default header */
    if ((resp_status = webclient_get(session, str_buf)) != 200)
    {
        LOG_E("webclient GET request failed, response(%d) error", resp_status);
        ret = -RT_ERROR;
        goto __exit;
    }
    bytes_read = webclient_read(session, (void *)buffer, GET_RESP_BUFSZ);
    rt_memset(buffer + bytes_read - 6, 0, 8); // buffer后面跟着0x0d 0x0a 0x30 0x0D 0x0A 0x0D 0x0A
    // {
    //     rt_kprintf("bytes_read : %d\n", bytes_read);
    //     for (int i = 0; i < bytes_read; i++)
    //     {
    //         rt_kprintf("%c", buffer[i]);
    //     }
    //     rt_kprintf("\n");
    // }
    LOG_I("get local weather");
    ret = weather_json_parser(buffer, p);
__exit:
    if (session)
    {
        webclient_close(session);
    }
    if (buffer)
    {
        web_free(buffer);
    }
    return ret;
}


struct APP_OBJ weather_app =
{
    .name = "Weather",
    .image = &app_weather,
    .info = "",
    .init = weather_init,
    .load = weather_load,
    .process = process,
    .exit_callback = exit_callback,
    .get_parm = get_weather_info_update_bitmap,
};
