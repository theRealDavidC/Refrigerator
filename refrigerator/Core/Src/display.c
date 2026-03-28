#include "display.h"
#include "ssd1306.h"
#include "config.h"
#include <stdio.h>

static const char *mode_str(fridge_mode_t mode)
{
    switch (mode) {
    case FRIDGE_IDLE:         return "IDLE";
    case FRIDGE_COOLING:      return "COOLING";
    case FRIDGE_DEFROST:      return "DEFROST";
    case FRIDGE_SENSOR_FAULT: return "SENSOR ERR";
    default:                  return "UNKNOWN";
    }
}

void display_init(void)
{
    ssd1306_init();
}

void display_update(const fridge_state_t *s)
{
    char buf[22];

    ssd1306_clear();

    // Title 
    ssd1306_draw_string(0, 0, "FRIDGE CONTROLLER", SSD1306_COLOR_WHITE);
    ssd1306_draw_line_h(0, 9, OLED_WIDTH_PX, SSD1306_COLOR_WHITE);

    // Temperature 
    snprintf(buf, sizeof(buf), "Temp:  %+5.1f C", (double)s->temperature_c);
    ssd1306_draw_string(0, 16, buf, SSD1306_COLOR_WHITE);

    // Target 
    snprintf(buf, sizeof(buf), "Target: %4.1f C", (double)FRIDGE_TARGET_TEMP_C);
    ssd1306_draw_string(0, 26, buf, SSD1306_COLOR_WHITE);

    // Mode 
    snprintf(buf, sizeof(buf), "Mode: %s", mode_str(s->mode));
    ssd1306_draw_string(0, 36, buf, SSD1306_COLOR_WHITE);

    ssd1306_draw_line_h(0, 47, OLED_WIDTH_PX, SSD1306_COLOR_WHITE);

    // Alarm line 
    if (s->alarms & ALARM_SENSOR_FAULT) {
        ssd1306_draw_string(0, 52, "!! SENSOR FAULT !!", SSD1306_COLOR_WHITE);
    } else if (s->alarms & ALARM_TEMP_HIGH) {
        ssd1306_draw_string(0, 52, "!! TEMP HIGH !!   ", SSD1306_COLOR_WHITE);
    } else if (s->alarms & ALARM_TEMP_LOW) {
        ssd1306_draw_string(0, 52, "!! TEMP LOW !!    ", SSD1306_COLOR_WHITE);
    } else {
        ssd1306_draw_string(0, 52, "Status: OK        ", SSD1306_COLOR_WHITE);
    }

    ssd1306_flush();
}
