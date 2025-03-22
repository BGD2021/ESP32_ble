#ifndef BLE2_HPP
#define BLE2_HPP

#include "config.h"


struct ble_device_info
{
    BLEUUID serviceUUID;
    BLEUUID charUUID[maxDevice];
};


#endif // BLE_HPP