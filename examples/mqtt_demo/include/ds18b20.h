#ifndef __DS18B20_H__
#define __DS18B20_H__

int ds1820_read(float *temperature);
static inline float rawToCelsius(int16_t raw);
char* Float2String(char* buffer, float value);

#endif
