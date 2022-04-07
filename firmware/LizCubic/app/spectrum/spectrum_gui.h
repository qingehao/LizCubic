#ifndef APP_SPECTRUM_GUI_H
#define APP_SPECTRUM_GUI_H
#include "stdint.h"
#include "lvgl.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* mindAnim */
#define MINDMAX 8
typedef enum
{
    ANIM_OK       = 0x00U,
    ANIM_ERROR    = 0x01U,
    ANIM_BUSY     = 0x02U,
    ANIM_IDLE     = 0x03U,
} ANIM_STATUS;
typedef struct
{
    float x;
    float y;
    float dirx;
    float diry;
    float r;
    uint32_t color;
} mind_node_t;

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint16_t x2;
    uint16_t y2;
	  uint16_t x1;
    uint16_t y1;
    uint8_t mind_num;
	uint8_t radius;
    mind_node_t mtmovmind[MINDMAX];
} mind_anim_t;

void spectrum_gui_init();
void spectrum_gui_load(lv_scr_load_anim_t anim_type);
void spectrum_gui_unload();

#define NPT  48
extern float target_height[NPT];
#ifdef __cplusplus
} /* extern "C" */
#endif


#endif