#include "alarm.h"
#include "buzzer.h"

static alarm_flags_t active_alarms = ALARM_NONE;

void alarm_init(void)
{
    active_alarms = ALARM_NONE;
}

void alarm_set(alarm_flags_t flags)
{
    alarm_flags_t prev = active_alarms;
    active_alarms |= flags;

    if (active_alarms == prev) return; // no change avoid re-triggering buzzer

    if (active_alarms & ALARM_SENSOR_FAULT) {
        buzzer_set_pattern(BUZZER_PATTERN_FAULT);
    } else if (active_alarms & (ALARM_TEMP_HIGH | ALARM_TEMP_LOW)) {
        buzzer_set_pattern(BUZZER_PATTERN_TEMP);
    }
}

void alarm_clear(alarm_flags_t flags)
{
    active_alarms &= ~flags;

    if (active_alarms == ALARM_NONE) {
        buzzer_set_pattern(BUZZER_PATTERN_NONE);
    }
}

alarm_flags_t alarm_get_active(void)
{
    return active_alarms;
}

int alarm_any_active(void)
{
    return active_alarms != ALARM_NONE;
}
