#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"

/* Temperature thresholds (Centigrade) */
#define FRIDGE_TARGET_TEMP_C        4.0f
#define FRIDGE_HYSTERESIS_C         1.0f      
#define FRIDGE_TEMP_HIGH_ALARM_C    12.0f    
#define FRIDGE_TEMP_LOW_ALARM_C    (-2.0f)   
#define FRIDGE_SENSOR_FAULT_RETRIES 3         

/* Compressor protection */
#define COMPRESSOR_MIN_OFF_SEC      180       // 3 min minimum off time
#define COMPRESSOR_MIN_ON_SEC       60        // 1 min minimum run time        

/* Defrost cycle*/
#define DEFROST_INTERVAL_SEC        28800     //8 hours between defrosts 
#define DEFROST_DURATION_SEC        1200      // 20 minute defrost run

/* Alarm timing */
#define ALARM_TEMP_CONFIRM_SEC      30        // confirm before raising alarm
#define BUZZER_BEEP_ON_MS           200
#define BUZZER_BEEP_OFF_MS          400

/* DS18B20 1-Wire GPIO*/
#define OW_GPIO_PORT                GPIOA
#define OW_GPIO_PIN                 GPIO_PIN_1

/*SSR compressor GPIO*/
#define SSR_GPIO_PORT               GPIOA
#define SSR_GPIO_PIN                GPIO_PIN_2
#define SSR_ACTIVE_LEVEL            GPIO_PIN_SET   // HIGH = compressor ON

/* Buzzer GPIO*/
#define BUZZER_GPIO_PORT            GPIOA
#define BUZZER_GPIO_PIN             GPIO_PIN_3
#define BUZZER_ACTIVE_LEVEL         GPIO_PIN_SET

/* SSD1306 OLED I2C*/
#define OLED_I2C_HANDLE             hi2c1
#define OLED_I2C_ADDR               0x78      // 7 bit addr 0x3C shifted left 
#define OLED_WIDTH_PX               128
#define OLED_HEIGHT_PX              64

/*Main loop interval*/
#define POLL_INTERVAL_MS            500

/* ESP32 UART expansion (future)*/
#define ESP32_UART_HANDLE           huart2
#define ESP32_UART_BAUD             115200

#endif 
