# Mini Refrigerator Controller STM32F103

## Hardware

| Component     | Pin      | Notes                                  |
|---------------|----------|----------------------------------------|
| DS18B20       | PA1      | 4.7 kΩ pull-up to 3.3 V required      |
| SSR input     | PA2      | HIGH = compressor ON                   |
| Buzzer        | PA3      | Active-high; add 100 Ω series resistor |
| OLED SDA      | PB7      | I2C1, 4.7 kΩ pull-up to 3.3 V         |
| OLED SCL      | PB6      | I2C1, 4.7 kΩ pull-up to 3.3 V         |
| ESP32 TX→RX   | PA3 (RX) | Future expansion share via jumper    |
| ESP32 RX←TX   | PA2 (TX) | Future expansion                       |

> DS18B20 VDD must go to 3.3 V, not 5 V, when powered from the STM32.


## Project Setup in STM32CubeIDE

### 1. Create a new STM32 project
- Open STM32CubeIDE → File → New → STM32 Project
- Select chip: **STM32F103C8Tx** (Blue Pill)
- Project name: `refrigerator`
- Language: C

### 2. Configure peripherals in CubeMX (.ioc file)

**RCC**
- HSE: Crystal/Ceramic Resonator

**Clock tree**
- PLL Source: HSE
- PLLMUL: ×9 -> SYSCLK = 72 MHz
- APB1 Prescaler: /2

**TIM2**
- Mode: Internal Clock
- Prescaler: `71` (72 MHz / (71+1) = 1 MHz -> 1 µs per tick)
- Period: `65535`
- No interrupt needed

**I2C1**
- Mode: I2C
- Speed: Fast Mode 400 kHz
- Pins: PB6 (SCL), PB7 (SDA)

**USART2** (optional, for ESP32)
- Mode: Asynchronous
- Baud: 115200
- Pins: PA2 (TX), PA3 (RX)

**GPIO**
- PA1: GPIO_Input (1-Wire is driven manually in ds18b20.c)
- PA2: GPIO_Output (SSR)
- PA3: GPIO_Output (Buzzer)
- PC13: GPIO_Output (onboard LED error indicator)

### 3. Copy source files

Copy all files from this repository into the generated project:

```
Core/Inc/  ← all .h files
Core/Src/  ← all .c files (replace generated main.c)
```

Do not let CubeMX regenerate `main.c` after you copy yours in, or use the
`USER CODE BEGIN / END` sections to protect your code during re-generation.

### 4. Add include path

Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths

Add: `${workspace_loc:/${ProjName}/Core/Inc}`

### 5. Build

- Right-click project → Build Project
- Confirm zero errors, zero warnings

---

## Flashing

### Using ST-Link V2
```
Project → Run → Debug Configurations → STM32 Cortex-M C/C++ Application
```

Or with OpenOCD from terminal:
```bash
openocd -f interface/stlink.cfg \
        -f target/stm32f1x.cfg \
        -c "program refrigerator.elf verify reset exit"
```

### Using STM32CubeProgrammer (GUI)
1. Connect ST-Link to SWDIO / SWCLK / GND / 3.3 V
2. Open CubeProgrammer → Connect → Open File → select `.elf`
3. Download → Disconnect

---

## Testing on the bench (before installing in fridge)

### Step 1: Power-on check
- Connect ST-Link and open a serial terminal at 115200 baud on USART2
- Power on the OLED should show `FRIDGE CONTROLLER` and `Status: OK`
- Confirm the SSR GPIO (PA2) reads LOW (compressor off)

### Step 2: Sensor check
- Plug DS18B20 with 4.7 kΩ pull-up
- Confirm the temperature reading on OLED matches a reference thermometer
- Pull the DS18B20 data line to GND to simulate a fault:
  - OLED should show `!! SENSOR FAULT !!`
  - Buzzer should sound fast pattern within 3 consecutive failed reads
  - SSR must be OFF during fault confirm with multimeter on PA2

### Step 3: Cooling trigger
- Warm the DS18B20 with your fingers above 5°C (target 4 + hysteresis 1)
- SSR GPIO (PA2) should go HIGH confirm with LED or multimeter
- Cool the sensor below 3°C SSR should go LOW after MIN_ON_SEC (60 s)

### Step 4: Short-cycle protection
- While compressor is OFF, immediately request cooling again
- Confirm that SSR stays LOW for 180 seconds (COMPRESSOR_MIN_OFF_SEC)
- OLED mode will show `COOLING` but SSR will only fire after lockout expires

### Step 5: High temperature alarm
- Heat sensor above 12°C (FRIDGE_TEMP_HIGH_ALARM_C) and hold for 30 seconds
- OLED should show `!! TEMP HIGH !!`
- Buzzer should start slow double-beep pattern

### Step 6: Defrost cycle (accelerated test)
- Temporarily change `DEFROST_INTERVAL_SEC` to `60` and rebuild
- After 60 seconds uptime the mode should switch to `DEFROST`
- Buzzer gives one short beep at defrost start
- SSR goes LOW during defrost regardless of temperature
- After `DEFROST_DURATION_SEC` the mode returns to `IDLE`
- Restore original value before production flash

---

## Adjusting thresholds

All tunable values are in `Core/Inc/config.h`. No other file needs to change.

| Constant                    | Default | Meaning                            |
|-----------------------------|---------|------------------------------------|
| FRIDGE_TARGET_TEMP_C        | 4.0     | Desired cabinet temperature        |
| FRIDGE_HYSTERESIS_C         | 1.0     | Band around target                 |
| FRIDGE_TEMP_HIGH_ALARM_C    | 12.0    | Alarm if temp rises above this     |
| FRIDGE_TEMP_LOW_ALARM_C     | -2.0    | Alarm if temp drops below this     |
| COMPRESSOR_MIN_OFF_SEC      | 180     | Minimum rest time between starts   |
| COMPRESSOR_MIN_ON_SEC       | 60      | Minimum run time per cycle         |
| DEFROST_INTERVAL_SEC        | 28800   | Time between forced defrosts (8 h) |
| DEFROST_DURATION_SEC        | 1200    | Length of each defrost (20 min)    |
| ALARM_TEMP_CONFIRM_SEC      | 30      | Delay before raising temp alarm    |

---

## Adding ESP32 (smart features)

When you are ready to add remote monitoring / Wi-Fi:

1. Connect ESP32 TX → STM32 PA3 (USART2 RX) and ESP32 RX → STM32 PA2 (TX)
2. Use `HAL_UART_Transmit(&huart2, ...)` in `fridge_tick()` to send a JSON
   status frame every N seconds, for example:
   ```json
   {"temp":4.2,"mode":"COOLING","alarm":0,"uptime":3600}
   ```
3. The ESP32 parses the frame and publishes to MQTT or an HTTP endpoint
4. No changes needed to any other module UART2 is already initialised
   and reserved in `main.c`

---

## File ownership map(Complete)

| File             | Responsibility                                      |
|------------------|-----------------------------------------------------|
| config.h         | All constants and pin assignments edit here only  |
| ds18b20.c        | 1-Wire read, CRC verify, temperature decode         |
| ssr.c            | SSR GPIO on/off                                     |
| compressor.c     | Short-cycle protection, ON/OFF state tracking       |
| buzzer.c         | Non-blocking pattern playback                       |
| ssd1306.c        | Raw I2C framebuffer driver                          |
| display.c        | OLED screen layout                                  |
| alarm.c          | Alarm flag management, buzzer coordination          |
| fridge.c         | Central state machine sensor → decision → output  |
| main.c           | HAL init, peripheral config, main loop              |
