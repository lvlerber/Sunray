
#ifndef WIREMUTEX_H
#define WIREMUTEX_H

#include <Arduino.h>
// #include <freertos/semphr.h>

#define I2C_BUS_COUNT 2
#define WIREMUTEX_TIMEOUT 255U

;
bool giveWire(uint8_t bus_num=0);
bool takeWire(uint8_t bus_num=0, uint8_t timeout=WIREMUTEX_TIMEOUT);

#endif