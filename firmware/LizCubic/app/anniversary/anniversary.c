#include "rtthread.h"
#include "rtdevice.h"
#include "anniversary_gui.h"
#include "anniversary.h"
#include "string.h"
#include "sys/time.h"
#include "liz_bsp.h"
#include "interface.h"
#include "network.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app.anniversary"
#if defined(LIZ_APP_WEATHER_DEBUG)
#define DBG_LVL    DBG_LOG
#else 
#define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

struct tm target_date; // 目标日
struct tm now_date;  
static uint8_t is_init = 0;

static int dateDiff(struct tm* date1, struct tm* date2);
static void get_date_diff();
static void set_target_date(int year, int mon, int day, int wday);

static void anniversary_init()
{
    if (is_init) return;
    is_init = 1;
    set_target_date(2022, 1, 1, 6);
    anniversary_gui_init();
}

static void anniversary_load(lv_scr_load_anim_t anim_type)
{
    get_date_diff();
    anniversary_gui_load(anim_type);
}

static void exit_callback()
{

}

static void process()
{

}

static void set_target_date(int year, int mon, int day, int wday)
{
    target_date.tm_year = year;
    target_date.tm_mon = mon;
    target_date.tm_mday = day;
    target_date.tm_wday = wday;
}

static void get_date_diff()
{
    time_t timep;
    struct tm *p_tm;
    time(&timep);
    p_tm = localtime(&timep); 
    
    now_date.tm_year = p_tm -> tm_year + 1900;
    now_date.tm_mon = p_tm -> tm_mon + 1;
    now_date.tm_mday = p_tm -> tm_mday;

    anniversary_day_count = dateDiff(&now_date, &target_date);
}

static void date_update()
{
    get_date_diff();
    anniversary_gui_display_date(&target_date);
}

static int dateDiff(struct tm* date1, struct tm* date2)
{
    int y1, m1, d1;
    int y2, m2, d2;
    m1 = (date1->tm_mon + 9) % 12;
    y1 = (date1->tm_year - m1/10);
    d1 = 365 * y1 + y1/4 -y1/100 + y1/400 + (m1*306+5)/10 + (date1->tm_mday - 1);

    m2 = (date2->tm_mon +9) % 12;
    y2 = date2->tm_year - m2/10;
    d2 = 365*y2 +y2/4 -y2/100 + y2/400 +(m2*306+5)/10 + (date2->tm_mday - 1);
    return (d2 -d1);
}

struct APP_OBJ anniversary_app =
{
    .name = "Anniversary",
    .image = RT_NULL,
    .info = "",
    .init = anniversary_init,
    .load = anniversary_load,
    .process = process,
    .exit_callback = exit_callback,
};
