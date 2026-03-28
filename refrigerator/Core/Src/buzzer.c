#include "buzzer.h"
#include "config.h"
#include "stm32f1xx_hal.h"

typedef struct {
    uint32_t on_ms;
    uint32_t off_ms;
    uint32_t repeats;    // 0 = continuous
} pattern_def_t;

static const pattern_def_t patterns[] = {
    [BUZZER_PATTERN_NONE]    = { 0,    0,    0 },
    [BUZZER_PATTERN_TEMP]    = { 200,  800,  0 },  
    [BUZZER_PATTERN_FAULT]   = { 100,  100,  0 },   
    [BUZZER_PATTERN_DEFROST] = { 300,  0,    1 },   
};

static buzzer_pattern_t active_pattern = BUZZER_PATTERN_NONE;
static int              buzzer_is_on   = 0;
static uint32_t         tick_counter   = 0;
static uint32_t         repeat_done    = 0;

static void buzzer_hw_on(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, BUZZER_ACTIVE_LEVEL);
    buzzer_is_on = 1;
}

static void buzzer_hw_off(void)
{
    GPIO_PinState off = (BUZZER_ACTIVE_LEVEL == GPIO_PIN_SET)
                        ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, off);
    buzzer_is_on = 0;
}

void buzzer_init(void)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = BUZZER_GPIO_PIN;
    cfg.Mode  = GPIO_MODE_OUTPUT_PP;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_GPIO_PORT, &cfg);
    buzzer_hw_off();
}

void buzzer_set_pattern(buzzer_pattern_t pattern)
{
    active_pattern = pattern;
    tick_counter   = 0;
    repeat_done    = 0;

    if (pattern == BUZZER_PATTERN_NONE) {
        buzzer_hw_off();
    } else {
        buzzer_hw_on();
    }
}

void buzzer_tick(void)
{
    const pattern_def_t *p = &patterns[active_pattern];

    if (active_pattern == BUZZER_PATTERN_NONE) return;

    // Single-shot pattern that has completed silence and stop 
    if (p->repeats > 0 && repeat_done >= p->repeats) {
        active_pattern = BUZZER_PATTERN_NONE;
        buzzer_hw_off();
        return;
    }

    tick_counter++;

    uint32_t period_ticks = (p->on_ms + p->off_ms) / POLL_INTERVAL_MS;
    if (period_ticks == 0) period_ticks = 1;

    uint32_t on_ticks  = p->on_ms  / POLL_INTERVAL_MS;
    uint32_t pos       = tick_counter % period_ticks;

    if (pos < on_ticks) {
        if (!buzzer_is_on) buzzer_hw_on();
    } else {
        if (buzzer_is_on) {
            buzzer_hw_off();
            repeat_done++;
        }
    }
}
