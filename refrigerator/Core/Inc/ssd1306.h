#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

typedef enum {
    SSD1306_COLOR_BLACK = 0,
    SSD1306_COLOR_WHITE = 1
} ssd1306_color_t;

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_flush(void);   // push framebuffer to display

void ssd1306_draw_char(uint8_t x, uint8_t y, char ch, ssd1306_color_t color);
void ssd1306_draw_string(uint8_t x, uint8_t y, const char *str, ssd1306_color_t color);
void ssd1306_draw_line_h(uint8_t x, uint8_t y, uint8_t len, ssd1306_color_t color);
void ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, ssd1306_color_t color);

#endif 
