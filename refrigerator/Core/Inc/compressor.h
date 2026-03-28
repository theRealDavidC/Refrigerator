#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <stdint.h>

typedef enum {
    COMPRESSOR_OFF   = 0,
    COMPRESSOR_ON    = 1,
    COMPRESSOR_LOCKOUT  // in min-off-time lockout cannot start yet 
} compressor_state_t;

void               compressor_init(void);
void               compressor_request_on(void);
void               compressor_request_off(void);
void               compressor_tick(void);    // call every POLL_INTERVAL_MS 
compressor_state_t compressor_get_state(void);
uint32_t           compressor_seconds_in_state(void);

#endif 
