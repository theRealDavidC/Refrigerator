#include "main.h"
#include "config.h"
#include "fridge.h"
#include "display.h"

// HAL peripheral handles (shared with driver modules via extern)
I2C_HandleTypeDef  hi2c1;
TIM_HandleTypeDef  htim2;
UART_HandleTypeDef huart2;  // reserved for future ESP32 expansion 

// Forward declarations 
static void system_clock_config(void);
static void gpio_init(void);
static void i2c1_init(void);
static void tim2_init(void);
static void uart2_init(void);
static void error_handler(void);


int main(void)
{
    HAL_Init();
    system_clock_config();

    gpio_init();
    i2c1_init();
    tim2_init();
    uart2_init();

    fridge_init();
    display_init();

    for (;;) {
        fridge_tick();
        display_update(fridge_get_state());
        HAL_Delay(POLL_INTERVAL_MS);
    }
}

// System clock — 72 MHz using HSE 8 MHz + PLL
static void system_clock_config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9;   // 8 MHz × 9 = 72 MHz           
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) error_handler();

    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                       | RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;   // APB1 max 36 MHz             
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) error_handler();
}

// GPIO enable clocks for all used ports 
static void gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /* Individual pin modes are configured by each driver module (ssr, buzzer,
     * ds18b20) at init time so ownership stays inside each driver.          */
}

// I2C1 PB6 SCL / PB7 SDA — 400 kHz for SSD1306 
static void i2c1_init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();

    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 400000;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) error_handler();

    /* Remap I2C1 pins to PB8/PB9 if using the alternate mapping:
      uncomment the line below and change pins in schematics accordingly.
      __HAL_AFIO_REMAP_I2C1_ENABLE();                                       */
}

// TIM2  1 µs tick for DS18B20 1-Wire timing 
static void tim2_init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = (uint32_t)(SystemCoreClock / 1000000) - 1;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 0xFFFF;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) error_handler();
}

// UART2 PA2 TX / PA3 RXreserved for ESP32 expansion
static void uart2_init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = ESP32_UART_BAUD;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) error_handler();
}

// HAL MSP callback low-level GPIO alternate function config
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1) {
        GPIO_InitTypeDef cfg = {0};
        cfg.Pin   = GPIO_PIN_6 | GPIO_PIN_7;   // SCL | SDA 
        cfg.Mode  = GPIO_MODE_AF_OD;
        cfg.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &cfg);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        GPIO_InitTypeDef cfg = {0};
        cfg.Pin   = GPIO_PIN_2;                 
        cfg.Mode  = GPIO_MODE_AF_PP;
        cfg.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &cfg);

        cfg.Pin  = GPIO_PIN_3;                  
        cfg.Mode = GPIO_MODE_INPUT;
        cfg.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &cfg);
    }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
    }
}

// Error handler halt and signal on LED (PA13 on Blue Pill)
static void error_handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(200);
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
