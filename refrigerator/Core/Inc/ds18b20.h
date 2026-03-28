#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

typedef enum {
    DS18B20_OK           = 0,
    DS18B20_ERR_NO_DEVICE,
    DS18B20_ERR_CRC,
    DS18B20_ERR_TIMEOUT
} ds18b20_status_t;

void ds18b20_init(void);

ds18b20_status_t ds18b20_read(float *out_celsius);

#endif
