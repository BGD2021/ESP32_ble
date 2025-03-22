#ifndef UTILS_HPP
#define UTILS_HPP

#include <Arduino.h>
#include "BLEDevice.h"

bool isFoundDevice(BLEAdvertisedDevice advertisedDevice);
float adcConvert2Liu(uint16_t adcValue);
#endif // UTILS_HPP
