#include <Arduino.h>
#include "BLEDevice.h"
#include "app.hpp"
#include "config.h"
// #include "BLEScan.h"

// 远程从机的UUID,这里修改记得同步修改app中的UUID
static BLEUUID serviceUUID1("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID1("0000fff1-0000-1000-8000-00805f9b34fb");
static BLEUUID char2UUID("0000fff2-0000-1000-8000-00805f9b34fb");

static BLEUUID serviceUUID2("00000000-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID2("00000001-0000-1000-8000-00805f9b34fb");
static BLEUUID char2UUID2("00000001-0000-1000-8000-00805f9b34fb");

static BLEUUID serviceUUID3("00000000-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID3("00000001-0000-1000-8000-00805f9b34fb");
static BLEUUID char2UUID3("00000001-0000-1000-8000-00805f9b34fb");
// 连接标志位
boolean doConnect = false;
boolean connected = false;
boolean doScan = false;

boolean isFoundDevice[3] = {false, false, false};

// 特征值和设备

BLERemoteCharacteristic *sRemoteCharacteristic;
BLERemoteCharacteristic *sRemoteCharacteristic2;
BLERemoteCharacteristic *sRemoteCharacteristic3;

BLECharacteristic *pCharacteristic;
BLECharacteristic *pCharacteristic2;

static BLEAdvertisedDevice *myDevice;
BLEAdvertising *beaconAdvertising;
BLEBeacon dataBeacon;
BLEServer *beaconServer;
// 多连接
BLERemoteCharacteristic *sCharacteristic[3][2];
static BLEAdvertisedDevice *Device[3];
boolean readyToConnect[3] = {false, false, false};
boolean connectedDevice[3] = {false, false, false};

// 作为从机的UUID
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// 队列
extern QueueHandle_t xADCQueue;
extern QueueHandle_t xSPIQueue;
extern QueueHandle_t xBLEQueue;

void printHex(std::string value)
{
  for (size_t i = 0; i < value.length(); i++)
  {
    Serial.print("0x");
    if ((uint8_t)value[i] < 0x10)
    {
      Serial.print("0"); // 添加前导零
    }
    Serial.print((uint8_t)value[i], HEX);
    if (i < value.length() - 1)
    {
      Serial.print(" ");
    }
  }
  Serial.println();
}

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  for (size_t i = 0; i < length; i++)
  {
    Serial.print("0x");
    if (pData[i] < 0x10)
    {
      Serial.print("0"); // 添加前导零
    }
    Serial.print(pData[i], HEX);
    if (i < length - 1)
    {
      Serial.print(" ");
    }
  }
  Serial.println();
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
  }

  void onDisconnect(BLEClient *pclient)
  {
    // 获取断开连接的从机地址
    BLEAddress disconnectedAddress = pclient->getPeerAddress();
    Serial.print("Disconnected from device: ");
    Serial.println(disconnectedAddress.toString().c_str());

    // 检查是哪一个从机断开连接
    for (int i = 0; i < 3; i++)
    {
      if (Device[i] != nullptr && Device[i]->getAddress().equals(disconnectedAddress))
      {
        connectedDevice[i] = false; // 标记该从机为未连接
        readyToConnect[i] = true;   // 标记该从机为未准备连接
        Serial.printf("Device %d disconnected\n", i);
        break;
      }
    }

    // 设置扫描标志位，重新扫描
    doScan = true;
  }
};

bool connectToServer()
{
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  if (!pClient->connect(myDevice))
  { // 检查连接是否成功
    Serial.println(" - Failed to connect to server");
    return false;
  }
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID1);
  // 防止连接失败，程序跑飞
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID[0].toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // 连接特征值
  sRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID[0]);
  if (sRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID[0].toString().c_str());
    pClient->disconnect();
    return false;
  }
  // sRemoteCharacteristic2 = pRemoteService->getCharacteristic(charUUID2);
  // if (sRemoteCharacteristic == nullptr) {
  //   Serial.print("Failed to find our characteristic UUID: ");
  //   Serial.println(charUUID2.toString().c_str());
  //   pClient->disconnect();
  //   return false;
  // }
  // Serial.println(" - Found our characteristic");

  // 首次读取特征值
  if (sRemoteCharacteristic->canRead())
  {
    std::string value = sRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }
  // if(sRemoteCharacteristic->canRead()) {
  //   std::string value = sRemoteCharacteristic->readValue();
  //   Serial.print("The characteristic value was: ");
  //   Serial.println(value.c_str());
  // }

  if (sRemoteCharacteristic->canNotify())
    sRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}

/*通过uuid连接从机*/
bool connectServerByUUID(uint8_t index, BLEUUID service_uuid, BLEUUID char_uuid, BLEUUID char2_uuid)
{
  Serial.print("Forming a connection to ");
  Serial.println(Device[index]->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());
  // Connect to the remove BLE Server.
  if (!pClient->connect(Device[index]))
  { // 检查连接是否成功
    Serial.println(" - Failed to connect to server");
    return false;
  }
  Serial.println(" - Connected to server");
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(service_uuid);
  // 防止连接失败，程序跑飞
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(service_uuid.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // 连接特征值
  sCharacteristic[index][0] = pRemoteService->getCharacteristic(char_uuid);
  if (sCharacteristic[index][0] == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(char_uuid.toString().c_str());
    pClient->disconnect();
    return false;
  }
  sCharacteristic[index][1] = pRemoteService->getCharacteristic(char2_uuid);
  if (sCharacteristic[index][1] == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(char_uuid.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if (sCharacteristic[index][0]->canRead())
  {
    std::string value = sCharacteristic[index][0]->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }
  connectedDevice[index] = true;
  return true;
}

/**
 * 根据serverUUID连接server
 */
class MultiAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());

    // 找到设备后，查找是否有我们需要的服务
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID1))
    {

      Serial.println("Found device0");
      Device[0] = new BLEAdvertisedDevice(advertisedDevice);
      isFoundDevice[0] = true;
      readyToConnect[0] = true;
    }
    else if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID2))
    {

      Device[1] = new BLEAdvertisedDevice(advertisedDevice);
      isFoundDevice[1] = true;
      readyToConnect[1] = true;
    }
    else if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID3))
    {

      Device[2] = new BLEAdvertisedDevice(advertisedDevice);
      isFoundDevice[2] = true;
      readyToConnect[2] = true;
    }
    else
    {
      // Serial.println("No service found");
      doScan = true;
    }
    if (isFoundDevice[0] && isFoundDevice[1] && isFoundDevice[2])
    {
      Serial.println("Found all devices");
      BLEDevice::getScan()->stop();
      doScan = false;
    }
  };
};

/*由于传参进去没有返回特征值，导致里面的特征值只作为局部变量使用，已废弃*/
// bool connect_to_serverByUUID(BLERemoteCharacteristic* Characteristic, BLEUUID service_uuid, BLEUUID char_uuid){
//     Serial.print("Forming a connection to ");
//     Serial.println(myDevice->getAddress().toString().c_str());
//     BLEClient*  pClient  = BLEDevice::createClient();
//     Serial.println(" - Created client");

//     pClient->setClientCallbacks(new MyClientCallback());

//     // Connect to the remove BLE Server.
//     if (!pClient->connect(myDevice)) {  // 检查连接是否成功
//         Serial.println(" - Failed to connect to server");
//         return false;
//     }
//     Serial.println(" - Connected to server");

//     // Obtain a reference to the service we are after in the remote BLE server.
//     BLERemoteService* pRemoteService = pClient->getService(service_uuid);
//     // 防止连接失败，程序跑飞
//     if (pRemoteService == nullptr) {
//       Serial.print("Failed to find our service UUID: ");
//       Serial.println(serviceUUID.toString().c_str());
//       pClient->disconnect();
//       return false;
//     }
//     Serial.println(" - Found our service");

//     //连接特征值
//     Characteristic = pRemoteService->getCharacteristic(char_uuid);
//     if (Characteristic == nullptr) {
//       Serial.print("Failed to find our characteristic UUID: ");
//       Serial.println(charUUID.toString().c_str());
//       pClient->disconnect();
//       return false;
//     }
//     Serial.println(" - Found our characteristic");
//     //首次读取特征值
//     if(Characteristic->canRead()) {
//       std::string value = Characteristic->readValue();
//       Serial.print("The characteristic value was: ");
//       Serial.println(value.c_str());
//     }
//     connected = true;
//     return true;
// }

/**
 * 根据serverUUID连接server
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());

    // 找到设备后，查找是否有我们需要的服务
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID1))
    {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
    }
    else
    {
      Serial.println("No service found");
      doScan = true;
    }
  }
};

void initBeacon()
{
  BLEDevice::init("ESP32");
  beaconServer = BLEDevice::createServer();
  // beaconServer->setCallbacks(new MyServerCallbacks());
  beaconAdvertising = beaconServer->getAdvertising();
  beaconAdvertising->stop();
  // iBeacon
  dataBeacon.setManufacturerId(0x4c00);
  dataBeacon.setMajor(5);
  dataBeacon.setMinor(88);
  dataBeacon.setSignalPower(0xc5);
  dataBeacon.setProximityUUID(BLEUUID(BEACON_UUID_REV));

  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x1A);
  advertisementData.setManufacturerData(dataBeacon.getData());
  beaconAdvertising->setAdvertisementData(advertisementData);

  // 设置扫描回应数据
  BLEAdvertisementData scanResponseData;
  scanResponseData.setName("ESP32iBeacon"); // 设置设备名称

  scanResponseData.setFlags(0x06);

  // 添加自定义数值到扫描回应数据
  uint8_t humidity = 30; // 假设湿度为60%
  std::string customResponseData;
  customResponseData.push_back(humidity); // 湿度数据
  customResponseData.push_back(0x11);     // 自定义数据类型标识符
  customResponseData.push_back(0x22);
  customResponseData.push_back(0x33); // 测试数值

  customResponseData.push_back(0x21); // 测试数值
  customResponseData.push_back(0x21); // 测试数值
  customResponseData.push_back(0x21); // 测试数值
  customResponseData.push_back(0x21); // 测试数值

  customResponseData.push_back(0x33); // 测试数值
  customResponseData.push_back(0x33); // 测试数值
  customResponseData.push_back(0x33); // 测试数值
  customResponseData.push_back(0x33); // 测试数值

  // 将自定义数据添加到扫描回应数据
  scanResponseData.addData(customResponseData);

  // 设置扫描回应数据
  beaconAdvertising->setScanResponseData(scanResponseData);

  beaconAdvertising->start();
}

void setBeaconMajor(uint16_t major)
{
  beaconAdvertising->stop();
  dataBeacon.setMajor(major);
  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x1A);
  advertisementData.setManufacturerData(dataBeacon.getData());
  beaconAdvertising->setAdvertisementData(advertisementData);
  beaconAdvertising->start();
}

void setBeaconMinor(uint16_t minor)
{
  beaconAdvertising->stop();
  dataBeacon.setMinor(minor);
  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x1A);
  advertisementData.setManufacturerData(dataBeacon.getData());
  beaconAdvertising->setAdvertisementData(advertisementData);
  beaconAdvertising->start();
}

void setBeaconData(uint8_t *dianliu, uint8_t *wendu, uint8_t *distance)
{
  beaconAdvertising->stop();
  // 设置扫描回应数据
  BLEAdvertisementData scanResponseData;
  scanResponseData.setName("ESP32iBeacon"); // 设置设备名称

  scanResponseData.setFlags(0x06);

  std::string customResponseData;
  customResponseData.push_back(dianliu[0]);
  customResponseData.push_back(wendu[0]);
  customResponseData.push_back(distance[0]);

  customResponseData.push_back(dianliu[1]);
  customResponseData.push_back(wendu[1]);
  customResponseData.push_back(distance[1]);

  customResponseData.push_back(dianliu[2]);
  customResponseData.push_back(wendu[2]);
  customResponseData.push_back(distance[2]);

  customResponseData.push_back(dianliu[3]);
  customResponseData.push_back(wendu[3]);
  customResponseData.push_back(distance[3]);

  // 将自定义数据添加到扫描回应数据
  scanResponseData.addData(customResponseData);

  // 设置扫描回应数据
  beaconAdvertising->setScanResponseData(scanResponseData);

  beaconAdvertising->start();
}

void ble_init()
{

  Serial.println("Starting BLE application...");
  initBeacon();

  // 主机初始化
  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan();
#ifdef MULTI_BLE
  pBLEScan->setAdvertisedDeviceCallbacks(new MultiAdvertisedDeviceCallbacks()); // 设置广告回调
#else
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); // 设置广告回调
#endif
  pBLEScan->setInterval(1349);   // 设置扫描间隔
  pBLEScan->setWindow(449);      // 设置扫描窗口
  pBLEScan->setActiveScan(true); // 设置是否主动扫描
  pBLEScan->start(5, false);     // 开始扫描

  Serial.println("BLE init OK");
}

void readSensor(uint8_t index)
{
  if (readyToConnect[index])
  {
    if (connectServerByUUID(index, serviceUUID[index], charUUID[0], charUUID[1]))
    {
      Serial.println("We are now connected to the BLE Server.");
    }
    else
    {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    readyToConnect[index] = false;
  }
  if (connectedDevice[index])
  {
    if (sCharacteristic[index][0]->canRead() && sCharacteristic[index][1]->canRead())
    {
      uint8_t value = sCharacteristic[index][0]->readUInt8();
      // uint8_t value2 = sCharacteristic[0][1]->readUInt8();
      // Serial.printf("value was: %d, value2 was: %d\r\n", value, value2);
      /*第一个特征值是整数部分，第二个特征值是小数部分*/
      // 拼数据
      //  float decimalPart = value2 / 256.0;
      //  float sensorData = value + decimalPart;

      // 添加到队列
      DataPacket packet;
      packet.type = index;
      packet.data = value;
      // packet.data = (uint32_t)(sensorData * 100);
      xQueueSend(xBLEQueue, &packet, portMAX_DELAY);
      // BLE_TASK_COPMLETE = true; // 任务完成
    }
  }
}
