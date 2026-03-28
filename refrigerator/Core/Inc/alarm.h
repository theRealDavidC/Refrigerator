#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>

typedef enum {
    ALARM_NONE         = 0x00,
    ALARM_TEMP_HIGH    = 0x01,
    ALARM_TEMP_LOW     = 0x02,
    ALARM_SENSOR_FAULT = 0x04
} alarm_flags_t;

void          alarm_init(void);
void          alarm_set(alarm_flags_t flags);
void          alarm_clear(alarm_flags_t flags);
alarm_flags_t alarm_get_active(void);
int           alarm_any_active(void);

#endif /* ALARM_H */
