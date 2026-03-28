#include "compressor.h"
#include "config.h"
#include "ssr.h"
#include "stm32f1xx_hal.h"

static compressor_state_t state          = COMPRESSOR_OFF;
static uint32_t           state_start_ms = 0;
static int                pending_on     = 0;

static uint32_t elapsed_ms(void)
{
    return HAL_GetTick() - state_start_ms;
}

void compressor_init(void)
{
    ssr_init();
    ssr_off();
    state          = COMPRESSOR_OFF;
    state_start_ms = HAL_GetTick();
    pending_on     = 0;
}

void compressor_request_on(void)
{
    pending_on = 1;
}

void compressor_request_off(void)
{
    pending_on = 0;

    if (state == COMPRESSOR_ON) {
        uint32_t on_sec = elapsed_ms() / 1000;
        if (on_sec >= COMPRESSOR_MIN_ON_SEC) {
            ssr_off();
            state          = COMPRESSOR_OFF;
            state_start_ms = HAL_GetTick();
        }
        // else: keep running until MIN_ON_SEC has elapsed 
    }
}

void compressor_tick(void)
{
    switch (state) {

    case COMPRESSOR_OFF:
        if (pending_on) {
            uint32_t off_sec = elapsed_ms() / 1000;
            if (off_sec >= COMPRESSOR_MIN_OFF_SEC) {
                ssr_on();
                state          = COMPRESSOR_ON;
                state_start_ms = HAL_GetTick();
            } else {
                state = COMPRESSOR_LOCKOUT;
            }
        }
        break;

    case COMPRESSOR_ON:
        // Nothing to enforce here — request_off() guards MIN_ON_SEC
        break;

    case COMPRESSOR_LOCKOUT:
        if (elapsed_ms() / 1000 >= COMPRESSOR_MIN_OFF_SEC) {
            if (pending_on) {
                ssr_on();
                state          = COMPRESSOR_ON;
                state_start_ms = HAL_GetTick();
            } else {
                state          = COMPRESSOR_OFF;
                state_start_ms = HAL_GetTick();
            }
        }
        break;
    }
}

compressor_state_t compressor_get_state(void)
{
    return state;
}

uint32_t compressor_seconds_in_state(void)
{
    return elapsed_ms() / 1000;
}
