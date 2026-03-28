#ifndef FRIDGE_H
#define FRIDGE_H

#include <stdint.h>
#include "alarm.h"

typedef enum {
    FRIDGE_IDLE,
    FRIDGE_COOLING,
    FRIDGE_DEFROST,
    FRIDGE_SENSOR_FAULT
} fridge_mode_t;

typedef struct {
    fridge_mode_t  mode;
    float          temperature_c;
    int            compressor_on;
    alarm_flags_t  alarms;
    uint32_t       uptime_sec;
} fridge_state_t;

void              fridge_init(void);
void              fridge_tick(void);
const fridge_state_t *fridge_get_state(void);

#endif 
