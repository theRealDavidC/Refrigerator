#include "ssr.h"
#include "config.h"
#include "stm32f1xx_hal.h"

static int ssr_state = 0;

void ssr_init(void)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = SSR_GPIO_PIN;
    cfg.Mode  = GPIO_MODE_OUTPUT_PP;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SSR_GPIO_PORT, &cfg);

    // Start with compressor off safe default 
    ssr_off();
}

void ssr_on(void)
{
    HAL_GPIO_WritePin(SSR_GPIO_PORT, SSR_GPIO_PIN, SSR_ACTIVE_LEVEL);
    ssr_state = 1;
}

void ssr_off(void)
{
    GPIO_PinState off_level = (SSR_ACTIVE_LEVEL == GPIO_PIN_SET)
                              ? GPIO_PIN_RESET
                              : GPIO_PIN_SET;
    HAL_GPIO_WritePin(SSR_GPIO_PORT, SSR_GPIO_PIN, off_level);
    ssr_state = 0;
}

int ssr_is_on(void)
{
    return ssr_state;
}
