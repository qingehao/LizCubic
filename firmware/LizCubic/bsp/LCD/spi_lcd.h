#ifndef __SPI_LCD_H__
#define __SPI_LCD_H__

#define LCD_W 240
#define LCD_H 240

int spi_lcd_init();
void lcd_address_set(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2);
void lcd_fill_array(rt_uint16_t x_start, rt_uint16_t y_start, rt_uint16_t x_end, rt_uint16_t y_end, void *pcolor);
void lcd_register_transmitCplt_callback(void (*callback)());
#endif
