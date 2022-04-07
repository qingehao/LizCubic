#ifndef GRADIENT_COLOR_H
#define GRADIENT_COLOR_H
#include "stdint.h"

typedef struct {
    uint32_t start_color;
    uint32_t end_color;
    uint16_t step_N;
    float  step_r;
    float  step_g;
    float  step_b;
    float cur_r;
    float cur_g;
    float cur_b;
    uint32_t  cur_color;
    uint16_t cnt;
} color_gradient_t ;
void color_gradient_init(color_gradient_t* gradient_t, uint32_t start_color, uint32_t end_color, uint16_t N);
uint32_t get_next_gradientColor(color_gradient_t* gradient_t);
uint32_t get_random_color();
#endif