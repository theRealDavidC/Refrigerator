#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

typedef enum {
    BUZZER_PATTERN_NONE    = 0,
    BUZZER_PATTERN_TEMP,      
    BUZZER_PATTERN_FAULT,     
    BUZZER_PATTERN_DEFROST    
} buzzer_pattern_t;

void buzzer_init(void);
void buzzer_set_pattern(buzzer_pattern_t pattern);
void buzzer_tick(void);   

#endif /* BUZZER_H */
