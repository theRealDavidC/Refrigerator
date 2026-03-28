#include "ds18b20.h"
#include "config.h"
#include "stm32f1xx_hal.h"

// DS18B20 ROM commands
#define CMD_SKIP_ROM        0xCC
#define CMD_CONVERT_T       0x44
#define CMD_READ_SCRATCHPAD 0xBE

// Scratchpad size and CRC polynomial 
#define SCRATCHPAD_BYTES    9
#define OW_CRC_POLY         0x8C

// Microsecond delay using TIM2
extern TIM_HandleTypeDef htim2;

static void delay_us(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

// 1-Wire GPIO helpers 
static void ow_pin_output(void)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = OW_GPIO_PIN;
    cfg.Mode  = GPIO_MODE_OUTPUT_OD;
    cfg.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OW_GPIO_PORT, &cfg);
}

static void ow_pin_input(void)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin  = OW_GPIO_PIN;
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(OW_GPIO_PORT, &cfg);
}

static void ow_low(void)  { HAL_GPIO_WritePin(OW_GPIO_PORT, OW_GPIO_PIN, GPIO_PIN_RESET); }
static void ow_high(void) { HAL_GPIO_WritePin(OW_GPIO_PORT, OW_GPIO_PIN, GPIO_PIN_SET);   }
static int  ow_read_bit(void) { return HAL_GPIO_ReadPin(OW_GPIO_PORT, OW_GPIO_PIN) == GPIO_PIN_SET; }

// 1-Wire reset returns 1 if device present
static int ow_reset(void)
{
    int presence;

    ow_pin_output();
    ow_low();
    delay_us(480);
    ow_high();

    ow_pin_input();
    delay_us(70);
    presence = (ow_read_bit() == 0);
    delay_us(410);

    return presence;
}

// Write a single bit 
static void ow_write_bit(int bit)
{
    ow_pin_output();
    ow_low();

    if (bit) {
        delay_us(1);
        ow_high();
        delay_us(60);
    } else {
        delay_us(60);
        ow_high();
        delay_us(1);
    }
}

// Read a single bit
static int ow_read_bit_op(void)
{
    int bit;

    ow_pin_output();
    ow_low();
    delay_us(1);

    ow_pin_input();
    delay_us(14);
    bit = ow_read_bit();
    delay_us(45);

    return bit;
}

// Write a full byte LSB first
static void ow_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

// Read a full byte LSB first
static uint8_t ow_read_byte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (ow_read_bit_op()) {
            byte |= (1 << i);
        }
    }
    return byte;
}

// CRC-8 / Maxim verify scratchpad integrity
static uint8_t crc8_maxim(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t b = 0; b < 8; b++) {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= OW_CRC_POLY;
            byte >>= 1;
        }
    }
    return crc;
}

// Public API 

void ds18b20_init(void)
{
    /* TIM2 must be configured for 1 MHz (1 µs per tick) in CubeMX:
     * Prescaler = (SystemCoreClock / 1000000) - 1
     * Period    = 0xFFFF
     * Start in free-running mode.                                           */
    HAL_TIM_Base_Start(&htim2);
}

ds18b20_status_t ds18b20_read(float *out_celsius)
{
    uint8_t scratchpad[SCRATCHPAD_BYTES];

    // Step 1: trigger temperature conversion
    if (!ow_reset()) return DS18B20_ERR_NO_DEVICE;

    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_CONVERT_T);

    // 12-bit conversion takes up to 750 ms poll until the line goes high  
    ow_pin_input();
    uint32_t start = HAL_GetTick();
    while (!ow_read_bit()) {
        if ((HAL_GetTick() - start) > 1000) {
            return DS18B20_ERR_TIMEOUT;
        }
    }

    // Step 2: read scratchpad 
    if (!ow_reset()) return DS18B20_ERR_NO_DEVICE;

    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_READ_SCRATCHPAD);

    for (int i = 0; i < SCRATCHPAD_BYTES; i++) {
        scratchpad[i] = ow_read_byte();
    }

    // Step 3: verify CRC 
    if (crc8_maxim(scratchpad, SCRATCHPAD_BYTES - 1) != scratchpad[8]) {
        return DS18B20_ERR_CRC;
    }

    // Step 4: decode temperature
    int16_t raw = (int16_t)((scratchpad[1] << 8) | scratchpad[0]);
    *out_celsius = (float)raw / 16.0f;

    return DS18B20_OK;
}
