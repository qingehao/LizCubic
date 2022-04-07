#include "spectrum_gui.h"
#include "color_gradient.h"
#include "liz_bsp.h"
#include "lvgl.h"
#include "liz_config.h"
#define DBG_TAG    "liz.app-gui.spectrum"
#if defined(LIZ_APP_SPECTRUM_DEBUG)
    #define DBG_LVL    DBG_LOG
#else
    #define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

#define SPECTRUM_LINE_WIDTH 3
#define SPECTRUM_LINE_STEP 6
#define MIND_RADIUS 6

static uint8_t is_running = 0;

static lv_style_t default_style;
static lv_style_t spectrum_style;
static lv_style_t mind_anim_style;


static lv_obj_t *scr = NULL;
static lv_obj_t *specturm_obj = NULL;
static lv_obj_t *back_anim_obj = NULL;

/* spectrum */
lv_timer_t *spectrum_update_timer = NULL;

float cur_height_arr[NPT] = {0}; // 当前高度
float target_height[NPT] = {0};  // 目标高度
float cur_height, new_height;
static float specturm_move_path(float cur, float target);
static void  spectrum_draw_event_cb(lv_event_t *e);
static void  spectrum_update_cb(lv_timer_t *timer);

/* mindAnim */
mind_anim_t mind_anim;
lv_timer_t *mind_anim_update_timer = NULL;
void mind_anim_init(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
static void mind_anim_draw_event_cb(lv_event_t *e);
static float get_mind_distanceSquare(float x0, float y0, float x1, float y1);
static ANIM_STATUS move_mind(uint8_t index);

static void display_back_anim();

/* 颜色渐变 */
color_gradient_t gradientColor1; //颜色渐变
color_gradient_t gradientColor2;
lv_color_t color;
uint32_t now_color;

void spectrum_gui_init(void)
{
    lv_style_reset(&default_style);
    lv_style_init(&default_style);
    lv_style_set_bg_color(&default_style,  lv_color_black());
    lv_style_set_border_width(&default_style, 0);

    lv_style_reset(&spectrum_style);
    lv_style_init(&spectrum_style);
    lv_style_set_bg_color(&spectrum_style,  lv_color_black());
    lv_style_set_border_width(&spectrum_style, 0);

    lv_style_reset(&mind_anim_style);
    lv_style_init(&mind_anim_style);
    lv_style_set_bg_color(&mind_anim_style,  lv_color_black());
    lv_style_set_border_width(&mind_anim_style, 0);
    lv_style_set_bg_opa(&mind_anim_style, LV_OPA_0);

    mind_anim_init(0, 40, 239, 200);
    color_gradient_init(&gradientColor1, 0XFF33CC, 0XFFA500, 16);
    color = lv_color_hex(0XFF33CC);
}

void spectrum_gui_load(lv_scr_load_anim_t anim_type)
{
    is_running = 0;
    scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &default_style, 0);

    /* 创建频谱绘制对象 */
    specturm_obj = lv_obj_create(scr);
    lv_obj_add_style(specturm_obj, &spectrum_style, 0);
    lv_obj_set_size(specturm_obj, 240, 160);
    lv_obj_clear_flag(specturm_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(specturm_obj, spectrum_draw_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(specturm_obj);

    /* 创建背景动画绘制对象 */
    back_anim_obj = lv_obj_create(scr);
    lv_obj_add_style(back_anim_obj, &mind_anim_style, 0);
    lv_obj_set_size(back_anim_obj, 240, 160);
    lv_obj_clear_flag(back_anim_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(back_anim_obj, mind_anim_draw_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(back_anim_obj);

    lv_obj_align(specturm_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(back_anim_obj, LV_ALIGN_CENTER, 0, 0);

    if (anim_type != LV_SCR_LOAD_ANIM_NONE)
    {
        lv_scr_load_anim(scr, anim_type, 500, 150, true);
        while (lv_anim_count_running() != 0) //当前页面有长文本滚动动画
        {
            lv_task_handler();
            rt_thread_mdelay(100);
        }
    }
    else
    {
        lv_scr_load(scr);
    }

    spectrum_update_timer = lv_timer_create(spectrum_update_cb, 20, NULL);
    lv_timer_set_repeat_count(spectrum_update_timer, -1);
    lv_timer_ready(spectrum_update_timer);

//    mind_anim_update_timer = lv_timer_create(back_anim_update_cb, 40, NULL);
//    lv_timer_set_repeat_count(mind_anim_update_timer, -1);
//    lv_timer_ready(mind_anim_update_timer);

    is_running = 1;
}

//计算路径增量
static float specturm_move_path(float cur, float target)
{
    float val = cur;
    float temp;
    temp = target - cur; //计算得到当前差值
    if (fabs(temp) <= 1e-6) return cur;
    if (temp > 0)
    {
        if (temp >= 30)
        {
            val = cur + 6.2;
        }
        else if (temp >= 20)
        {
            val = cur + 5.8;
        }
        else if (temp >= 10)
        {
            val = cur + 2.8;
        }
        else if (temp >= 5)
        {
            val = cur + 2.0;
        }
        else if (temp >= 4)
        {
            val = cur + 1.0;
        }
        else if (temp >= 3)
        {
            val = cur + 0.1;
        }
        else if (temp >= 2)
        {
            val = cur + 0.1;
        }
        else if (temp >= 0.5)
        {
            val = cur;
        }
    }
    else
    {
        if (temp <= -30)
        {
            val = cur - 3.2;
        }
        else if (temp <= -15)
        {
            val = cur - 2.2;
        }
        else if (temp <= -10)
        {
            val = cur - 1.2;
        }
        else if (temp <= -5)
        {
            val = cur - 0.4;
        }
        else if (temp <= -3)
        {
            val = cur - 0.3;
        }
        else if (temp <= -2)
        {
            val = cur - 0.2;
        }
        else if (temp <= -1)
        {
            val = cur - 0.1;
        }
        else if (temp <= -0.5)
        {
            val = cur;
        }
    }
    return val;
}

static void spectrum_draw_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_COVER_CHECK)
    {
        lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
    }
    else if (code == LV_EVENT_DRAW_POST)
    {
        lv_obj_t *obj = lv_event_get_target(e);
        const lv_area_t *clip_area = lv_event_get_param(e);
        lv_opa_t opa = lv_obj_get_style_opa(obj, LV_PART_MAIN);
        uint8_t i = 0;
        lv_point_t center;
        lv_point_t amp_points[2];
        lv_point_t old_amp_points[2];
        // 绘制上半部分波形
        lv_draw_line_dsc_t draw_dsc;
        lv_draw_line_dsc_init(&draw_dsc);
        draw_dsc.width = SPECTRUM_LINE_WIDTH;
        draw_dsc.color = color;
        draw_dsc.round_start = 0;
        // 绘制下半部分波形
        lv_draw_line_dsc_t draw_dsc_1;
        lv_draw_line_dsc_init(&draw_dsc_1);
        draw_dsc_1.width = SPECTRUM_LINE_WIDTH;
        draw_dsc_1.round_end = 0;
        draw_dsc_1.round_start = 0;
        draw_dsc_1.color = color;
        // 获得中心点坐标
        center.x = obj->coords.x1 + (lv_obj_get_width(obj) >> 1);
        center.y = obj->coords.y1 + (lv_obj_get_height(obj) >> 1);

        for (i = 0; i < NPT; i++)
        {
            cur_height = cur_height_arr[i];
            new_height = specturm_move_path(cur_height, target_height[i]); //获得新高度
            cur_height_arr[i] = new_height;

            amp_points[0].x = obj->coords.x1 + SPECTRUM_LINE_STEP * i ;
            amp_points[0].y = (int)(center.y + new_height);
            amp_points[1].x = obj->coords.x1 + SPECTRUM_LINE_STEP * i ;
            amp_points[1].y = (int)(center.y - new_height);

            lv_draw_line(&amp_points[0], &amp_points[1], clip_area, &draw_dsc);
            if (i > 0)
            {
                lv_draw_line(&old_amp_points[0], &amp_points[0], clip_area, &draw_dsc);
                lv_draw_line(&old_amp_points[1], &amp_points[1], clip_area, &draw_dsc);
            }
            old_amp_points[0] = amp_points[0];
            old_amp_points[1] = amp_points[1];
        }
    }
}

void mind_anim_init(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    int i = 0;
    mind_anim.x1 = x1;
    mind_anim.y1 = y1;
    mind_anim.x2 = x2;
    mind_anim.y2 = y2;
    mind_anim.width = x2 - x1;
    mind_anim.height = y2 - y1;
    mind_anim.radius = MIND_RADIUS;
    mind_anim.mind_num = 5;
    for (i = 0; i < MINDMAX; i++)
    {
        mind_anim.mtmovmind[i].color = get_random_color();
        mind_anim.mtmovmind[i].x = rand() % (mind_anim.x2 - mind_anim.x1) + mind_anim.radius + mind_anim.x1;
        __asm("nop");
        mind_anim.mtmovmind[i].y = rand() % (mind_anim.y2 - mind_anim.y1) + mind_anim.radius + mind_anim.y1;
        mind_anim.mtmovmind[i].dirx = (rand() % 30 - 15) * 0.1f;
        mind_anim.mtmovmind[i].diry = (rand() % 30 - 15) * 0.1f;
        if (mind_anim.mtmovmind[i].dirx < 0.2 && mind_anim.mtmovmind[i].dirx > -0.2f)
        {
            mind_anim.mtmovmind[i].dirx = 0.5f;
        }
        if (mind_anim.mtmovmind[i].diry < 0.2f && mind_anim.mtmovmind[i].diry > -0.2f)
        {
            mind_anim.mtmovmind[i].diry = 0.5f;
        }
    }
}

static float get_mind_distanceSquare(float x0, float y0, float x1, float y1)
{
    return ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
}

static ANIM_STATUS move_mind(uint8_t index)
{
    if (mind_anim.mtmovmind[index].x <= mind_anim.x1 + mind_anim.radius)
    {
        mind_anim.mtmovmind[index].x = mind_anim.x1 + mind_anim.radius + 2;
        return ANIM_IDLE;
    }
    else if (mind_anim.mtmovmind[index].x >= mind_anim.x2 - mind_anim.radius)
    {
        mind_anim.mtmovmind[index].x = mind_anim.x2 - mind_anim.radius - 2;
        return ANIM_IDLE;
    }
    else if (mind_anim.mtmovmind[index].y <= mind_anim.y1 + mind_anim.radius)
    {
        mind_anim.mtmovmind[index].y = mind_anim.y1 + mind_anim.radius + 2;
        return ANIM_IDLE;
    }
    else if (mind_anim.mtmovmind[index].y >= mind_anim.y2 - mind_anim.radius)
    {
        mind_anim.mtmovmind[index].y = mind_anim.y2 - mind_anim.radius - 2;
        return ANIM_IDLE;
    }
    else
    {
        mind_anim.mtmovmind[index].x += mind_anim.mtmovmind[index].dirx;
        mind_anim.mtmovmind[index].y += mind_anim.mtmovmind[index].diry;
    }
    return ANIM_BUSY;
}

static void mind_anim_draw_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_COVER_CHECK)
    {
        lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
    }
    else if (code == LV_EVENT_DRAW_POST)
    {

        lv_obj_t *obj = lv_event_get_target(e);
        const lv_area_t *clip_area = lv_event_get_param(e);
        lv_opa_t opa = lv_obj_get_style_opa(obj, LV_PART_MAIN);

        int i, j;
        lv_area_t circle_coords;
        lv_draw_rect_dsc_t draw_circle_dsc;
        lv_draw_rect_dsc_init(&draw_circle_dsc);
        draw_circle_dsc.radius = mind_anim.radius;
        lv_point_t line_points[2];
        lv_draw_line_dsc_t draw_line_dsc;
        lv_draw_line_dsc_init(&draw_line_dsc);
        draw_line_dsc.round_end = 0;
        draw_line_dsc.round_start = 0;
        draw_line_dsc.width = (mind_anim.radius / 2 + 2);
        for (i = 0; i < mind_anim.mind_num; i++)
        {
            if (move_mind(i) == ANIM_IDLE)
            {
                mind_anim.mtmovmind[i].color = get_random_color();
                mind_anim.mtmovmind[i].dirx = (rand() % 30 - 15) * 0.1f;
                mind_anim.mtmovmind[i].diry = (rand() % 30 - 15) * 0.1f;
                if (mind_anim.mtmovmind[i].dirx < 0.2 && mind_anim.mtmovmind[i].dirx > -0.2)
                    mind_anim.mtmovmind[i].dirx = 0.5;
                if (mind_anim.mtmovmind[i].diry < 0.2 && mind_anim.mtmovmind[i].diry > -0.2)
                    mind_anim.mtmovmind[i].diry = 0.5;
                if (mind_anim.mind_num < MINDMAX)
                {
                    mind_anim.mind_num++;
                }
            }
        }
        for (i = 0; i < mind_anim.mind_num; i++)
        {
            for (j = 0; j < mind_anim.mind_num; j++)
            {
                if (get_mind_distanceSquare(mind_anim.mtmovmind[i].x, mind_anim.mtmovmind[i].y, mind_anim.mtmovmind[j].x, mind_anim.mtmovmind[j].y) < 1200)
                {
                    draw_line_dsc.color = color; //LV_COLOR_BLUE; lv_color_hex(RandomColor())
                    line_points[0].x = mind_anim.mtmovmind[j].x;
                    line_points[0].y = mind_anim.mtmovmind[j].y;
                    line_points[1].x = mind_anim.mtmovmind[i].x;
                    line_points[1].y = mind_anim.mtmovmind[i].y;
                    lv_draw_line(&line_points[0], &line_points[1], clip_area, &draw_line_dsc);
                }
            }
        }
        for (i = 0; i < mind_anim.mind_num; i++)
        {
            draw_circle_dsc.bg_color = lv_color_hex(mind_anim.mtmovmind[i].color);
            circle_coords.x1 = mind_anim.mtmovmind[i].x - mind_anim.radius;
            circle_coords.y1 = mind_anim.mtmovmind[i].y - mind_anim.radius;
            circle_coords.x2 = mind_anim.mtmovmind[i].x + mind_anim.radius;
            circle_coords.y2 = mind_anim.mtmovmind[i].y + mind_anim.radius;
            lv_draw_rect(&circle_coords, clip_area, &draw_circle_dsc);
        }

    }
}

void display_spectrum()
{
    static uint16_t spectrum_update_timer_cnt = 0;
    int i = 0;
    spectrum_update_timer_cnt++;
    lv_obj_invalidate(specturm_obj);
    if (spectrum_update_timer_cnt > 20)
    {
        spectrum_update_timer_cnt = 0;
        now_color = get_next_gradientColor(&gradientColor1);
        color = lv_color_hex(now_color);
    }
}

static void spectrum_update_cb(lv_timer_t *timer)
{
    static uint16_t spectrum_update_timer_cnt = 0;
    if(spectrum_update_timer_cnt % 2 == 0) // 每40ms更新一次背景动画
    {
        lv_obj_invalidate(back_anim_obj);
    }
    if (is_running)
    {
        display_spectrum();
    }
}

static void display_back_anim()
{
    lv_obj_invalidate(back_anim_obj);
}

static void back_anim_update_cb(lv_timer_t *timer)
{
    if (is_running)
    {
        display_back_anim();
    }
}


void spectrum_gui_unload()
{
    is_running = 0;
    lv_timer_del(spectrum_update_timer);
//    lv_timer_del(mind_anim_update_timer);
}

