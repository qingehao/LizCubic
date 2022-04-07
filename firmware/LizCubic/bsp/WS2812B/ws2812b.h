#ifndef __WS2812B_H__
#define __WS2812B_H__
#include "stdint.h"

#define WS1  90 // 65
#define WS0  28 // 25
#define RGB_LED_ALL -1

#define RGB_LED_COLOR_RED     0X600000
#define RGB_LED_COLOR_GREEN   0X006000
#define RGB_LED_COLOR_BLUE    0X000060

enum RGB_LED_MODE{
    RGB_LED_MODE_BLINK = 0, // 闪烁模式
    RGB_LED_MODE_BREATHE,    // 呼吸模式
    RGB_LED_MODE_FADEOUT,    // 呼吸模式
    RGB_LED_MODE_HOLDLIGHT, // 常亮模式
    RGB_LED_MODE_HOLDOFF,  // 
    RGB_LED_MODE_NONE,
};

union ws_color
{
    struct{
        uint8_t b;
        uint8_t g;
        uint8_t r;
    };
    uint32_t hex;
};

struct ws2812_rgbled
{
    uint16_t led_num; //led灯的数量
    int (*set_color)(int index,uint32_t c); // 设置颜色
    int (*set_mode)(enum RGB_LED_MODE mode);  // 设置模式
    void (*set_blink_period)(int period); // 设置闪烁周期
    void (*set_breathe_period)(int period); // 设置呼吸周期
    void (*clear)(void);
    union ws_color color;
    int blink_period;
    int breathe_period;
    void (*start)(void);
    void (*stop)(void);
    uint8_t is_init;
};

typedef struct ws2812_rgbled* ws2812_rgbled_t;

ws2812_rgbled_t ws2812b_rgb_led_init(uint16_t led_num);


#endif
