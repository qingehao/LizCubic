#include "color_gradient.h"

void color_gradient_init(color_gradient_t* gradient_t, uint32_t start_color, uint32_t end_color, uint16_t N)
{
    gradient_t->start_color = start_color;
    gradient_t->end_color = end_color;
    gradient_t->step_N = N;
    gradient_t->cur_color = start_color;
    float r_1 = (start_color >> 16) & 0xFF;
    float g_1 = (start_color >> 8) & 0xFF;
    float b_1 = (start_color >> 0) & 0xFF;

    float r_2 = (end_color >> 16) & 0xFF;
    float g_2 = (end_color >> 8) & 0xFF;
    float b_2 = (end_color >> 0) & 0xFF;
    gradient_t->cur_r = r_1;
    gradient_t->cur_g = g_1;
    gradient_t->cur_b = b_1;
    gradient_t->step_r = (r_2 - r_1) / N;
    gradient_t->step_g = (g_2 - g_1) / N;
    gradient_t->step_b = (b_2 - b_1) / N;
    gradient_t->cnt = 0;
}
uint32_t get_next_gradientColor(color_gradient_t* gradient_t)
{
    gradient_t->cur_r += gradient_t->step_r;
    gradient_t->cur_g += gradient_t->step_g;
    gradient_t->cur_b += gradient_t->step_b;
    gradient_t->cnt++;
    if (gradient_t->cnt == gradient_t->step_N)
    {
        gradient_t->step_r = -gradient_t->step_r;
        gradient_t->step_g = -gradient_t->step_g;
        gradient_t->step_b = -gradient_t->step_b;
        gradient_t->cnt = 0;
    }
    gradient_t->cur_color = (uint32_t)((uint32_t)gradient_t->cur_r << 16 | (uint32_t)gradient_t->cur_g << 8 | (uint32_t)gradient_t->cur_b);
    return gradient_t->cur_color;
}
uint32_t get_random_color()
{
	uint8_t red,green,blue;
	red = rand();
	__asm("nop");
	green = rand();
	__asm("nop");
	blue = rand();
	return (red<<16|green<<8|blue);
}