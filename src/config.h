#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>
#include "BLEDevice.h"

#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEBeacon.h>

const static int maxDevice = 3;
const static int maxChar = 4;

// 使用 extern 声明变量
extern BLEUUID serviceUUID[];
extern BLEUUID charUUID[];

// const BLEUUID serviceUUID1("0000fff0-0000-1000-8000-00805f9b34fb");
// const BLEUUID    char_uuid1("0000fff1-0000-1000-8000-00805f9b34fb");
// const BLEUUID    char2_uuid1("0000fff2-0000-1000-8000-00805f9b34fb");
// //远程从机的UUID
// const BLEUUID serviceUUID2("0000fff0-0000-1000-8000-00805f9b34fb");
// const BLEUUID    char_uuid2("0000fff1-0000-1000-8000-00805f9b34fb");
// const BLEUUID    char2_uuid2("0000fff1-0000-1000-8000-00805f9b34fb");
// //远程从机的UUID
// const BLEUUID serviceUUID3("0000fff0-0000-1000-8000-00805f9b34fb");
// const BLEUUID    char_uuid3("0000fff1-0000-1000-8000-00805f9b34fb");
// const BLEUUID    char2_uuid3("0000fff1-0000-1000-8000-00805f9b34fb");

#define BEACON_UUID         "2D7A9F0C-E0E8-4CC9-A71B-A21DB2D034A1"
#define BEACON_UUID_REV     "A134D0B2-1DA2-1BA7-C94C-E8E00C9F7A2D"


#endif