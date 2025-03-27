#ifndef BLE_HPP
#define BLE_HPP

#include <Arduino.h>
#include "BLEDevice.h"
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEBeacon.h>
// 从机的UUID
// extern BLEUUID serviceUUID;
// extern BLEUUID charUUID;
// extern BLEUUID charUUID2;

// 连接标志位
extern boolean doConnect;
extern boolean connected;
extern boolean doScan;

// 特征值和设备
extern BLERemoteCharacteristic* pRemoteCharacteristic;
extern BLERemoteCharacteristic* pRemoteCharacteristic2;
extern BLEAdvertisedDevice* myDevice;

// 主机的UUID
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

void printHex(std::string value);
void ble_init();
void ble_task(void *pvParameter);
bool connectToServer();
bool connectServerByUUID(uint8_t index,BLEUUID service_uuid, BLEUUID char_uuid, BLEUUID char2_uuid);
void BLESendTask(void *pvParameters);
void readSensor(uint8_t index);
void setBeaconMajor(uint16_t major);
void setBeaconMinor(uint16_t minor);
void setBeaconData(uint8_t *dianliu, uint8_t *wendu, uint8_t *distance);

#endif // BLE_HPP
