#include "fridge.h"
#include "config.h"
#include "ds18b20.h"
#include "compressor.h"
#include "alarm.h"
#include "buzzer.h"
#include "stm32f1xx_hal.h"

static fridge_state_t ctx;
static uint32_t       sensor_fault_count = 0;
static uint32_t       defrost_last_sec   = 0;
static uint32_t       defrost_start_sec  = 0;
static uint32_t       temp_alarm_since   = 0;
static uint32_t       boot_tick          = 0;

// Helpers 
static uint32_t now_sec(void)
{
    return (HAL_GetTick() - boot_tick) / 1000;
}

static void enter_mode(fridge_mode_t mode)
{
    ctx.mode = mode;
}

// Sensor read with fault counting
static int read_temperature(void)
{
    float temp;
    ds18b20_status_t status = ds18b20_read(&temp);

    if (status == DS18B20_OK) {
        ctx.temperature_c  = temp;
        sensor_fault_count = 0;
        return 1;
    }

    sensor_fault_count++;
    return 0;
}

// Evaluate temperature alarms 
static void evaluate_temp_alarms(void)
{
    int too_hot  = ctx.temperature_c > FRIDGE_TEMP_HIGH_ALARM_C;
    int too_cold = ctx.temperature_c < FRIDGE_TEMP_LOW_ALARM_C;

    if (too_hot || too_cold) {
        if (temp_alarm_since == 0) {
            temp_alarm_since = now_sec();
        } else if ((now_sec() - temp_alarm_since) >= ALARM_TEMP_CONFIRM_SEC) {
            if (too_hot)  alarm_set(ALARM_TEMP_HIGH);
            if (too_cold) alarm_set(ALARM_TEMP_LOW);
        }
    } else {
        temp_alarm_since = 0;
        alarm_clear(ALARM_TEMP_HIGH | ALARM_TEMP_LOW);
    }
}

// Cooling decision hysteresis band 
static void evaluate_cooling(void)
{
    float high_thresh = FRIDGE_TARGET_TEMP_C + FRIDGE_HYSTERESIS_C;
    float low_thresh  = FRIDGE_TARGET_TEMP_C - FRIDGE_HYSTERESIS_C;

    if (ctx.mode == FRIDGE_IDLE && ctx.temperature_c >= high_thresh) {
        compressor_request_on();
        enter_mode(FRIDGE_COOLING);
    } else if (ctx.mode == FRIDGE_COOLING && ctx.temperature_c <= low_thresh) {
        compressor_request_off();
        enter_mode(FRIDGE_IDLE);
    }
}

// Defrost cycle 
static void evaluate_defrost(void)
{
    uint32_t t = now_sec();

    if (ctx.mode == FRIDGE_DEFROST) {
        if ((t - defrost_start_sec) >= DEFROST_DURATION_SEC) {
            compressor_request_off();
            defrost_last_sec = t;
            enter_mode(FRIDGE_IDLE);
        }
        return;
    }

    if ((t - defrost_last_sec) >= DEFROST_INTERVAL_SEC) {
        compressor_request_off();
        defrost_start_sec = t;
        buzzer_set_pattern(BUZZER_PATTERN_DEFROST);
        enter_mode(FRIDGE_DEFROST);
    }
}

// Public API

void fridge_init(void)
{
    boot_tick         = HAL_GetTick();
    defrost_last_sec  = 0;
    sensor_fault_count = 0;
    temp_alarm_since  = 0;

    ctx.mode           = FRIDGE_IDLE;
    ctx.temperature_c  = 0.0f;
    ctx.compressor_on  = 0;
    ctx.alarms         = ALARM_NONE;
    ctx.uptime_sec     = 0;

    ds18b20_init();
    compressor_init();
    alarm_init();
    buzzer_init();
}

void fridge_tick(void)
{
    ctx.uptime_sec = now_sec();

    // 1. Read sensor 
    if (!read_temperature()) {
        if (sensor_fault_count >= FRIDGE_SENSOR_FAULT_RETRIES) {
            alarm_set(ALARM_SENSOR_FAULT);
            compressor_request_off();   // safety: stop compressor on fault  
            enter_mode(FRIDGE_SENSOR_FAULT);
        }
    } else {
        if (ctx.mode == FRIDGE_SENSOR_FAULT) {
            alarm_clear(ALARM_SENSOR_FAULT);
            enter_mode(FRIDGE_IDLE);
        }
    }

    // 2. Skip cooling/defrost logic when faulted
    if (ctx.mode == FRIDGE_SENSOR_FAULT) {
        compressor_tick();
        buzzer_tick();
        ctx.compressor_on = (compressor_get_state() == COMPRESSOR_ON);
        ctx.alarms        = alarm_get_active();
        return;
    }

    // 3. Defrost overrides cooling
    evaluate_defrost();

    // 4. Normal cooling band
    if (ctx.mode != FRIDGE_DEFROST) {
        evaluate_cooling();
    }

    // 5. Temperature alarm evaluation 
    evaluate_temp_alarms();

    // 6. Drive hardware 
    compressor_tick();
    buzzer_tick();

    ctx.compressor_on = (compressor_get_state() == COMPRESSOR_ON);
    ctx.alarms        = alarm_get_active();
}

const fridge_state_t *fridge_get_state(void)
{
    return &ctx;
}
