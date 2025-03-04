#include <Arduino.h>
#include "BLEDevice.h"
#include "app.hpp"
//#include "BLEScan.h"

//远程从机的UUID,这里修改记得同步修改app中的UUID
static BLEUUID serviceUUID1("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID    charUUID1("0000fff1-0000-1000-8000-00805f9b34fb");

static BLEUUID serviceUUID2("00000000-0000-1000-8000-00805f9b34fb");
static BLEUUID    charUUID2("00000001-0000-1000-8000-00805f9b34fb");


static BLEUUID serviceUUID3("00000000-0000-1000-8000-00805f9b34fb");
static BLEUUID    charUUID3("00000001-0000-1000-8000-00805f9b34fb");
//连接标志位
boolean doConnect = false;
boolean connected = false;
boolean doScan = false;

boolean isFoundDevice[3] = {false, false, false};

//特征值和设备

BLERemoteCharacteristic* sRemoteCharacteristic;
BLERemoteCharacteristic* sRemoteCharacteristic2;
BLERemoteCharacteristic* sRemoteCharacteristic3;

BLECharacteristic *pCharacteristic;
BLECharacteristic *pCharacteristic2;

static BLEAdvertisedDevice* myDevice;

//多连接
BLERemoteCharacteristic* sCharacteristic[3];
static BLEAdvertisedDevice* Device[3];
boolean readyToConnect[3] = {false, false, false};
boolean connectedDevice[3] = {false, false, false};

//作为从机的UUID
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// 队列
extern QueueHandle_t xADCQueue;
extern QueueHandle_t xSPIQueue;
extern QueueHandle_t xBLEQueue;

void printHex(std::string value){
  for (size_t i = 0; i < value.length(); i++) {
    Serial.print("0x");
    if ((uint8_t)value[i] < 0x10) {
        Serial.print("0");  // 添加前导零
    }
    Serial.print((uint8_t)value[i], HEX);
    if (i < value.length() - 1) {
        Serial.print(" ");
    }
  }
  Serial.println();
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    for (size_t i = 0; i < length; i++) {
        Serial.print("0x");
        if (pData[i] < 0x10) {
            Serial.print("0");  // 添加前导零
        }
        Serial.print(pData[i], HEX);
        if (i < length - 1) {
            Serial.print(" ");
        }
    }
    Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    //TODO:对每个设备都加入断开连接的处理
    connected = false;
    Serial.println("onDisconnect");
    doScan = true;
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    if (!pClient->connect(myDevice)) {  // 检查连接是否成功
        Serial.println(" - Failed to connect to server");
        return false;
    }
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID1);
    // 防止连接失败，程序跑飞
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID1.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    //连接特征值
    sRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID1);
    if (sRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID1.toString().c_str());
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

    //首次读取特征值
    if(sRemoteCharacteristic->canRead()) {
      std::string value = sRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
    // if(sRemoteCharacteristic->canRead()) {
    //   std::string value = sRemoteCharacteristic->readValue();
    //   Serial.print("The characteristic value was: ");
    //   Serial.println(value.c_str());
    // }

    if(sRemoteCharacteristic->canNotify())
      sRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}


/*通过uuid连接从机*/
bool connectServerByUUID(uint8_t index,BLEUUID service_uuid, BLEUUID char_uuid){
  Serial.print("Forming a connection to ");
  Serial.println(Device[index]->getAddress().toString().c_str());
  
  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());
  // Connect to the remove BLE Server.
  if (!pClient->connect(Device[index])) {  // 检查连接是否成功
      Serial.println(" - Failed to connect to server");
      return false;
  }
  Serial.println(" - Connected to server");
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(service_uuid);
  // 防止连接失败，程序跑飞
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(service_uuid.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  //连接特征值
  sCharacteristic[index] = pRemoteService->getCharacteristic(char_uuid);
  if (sCharacteristic[index] == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(char_uuid.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if(sCharacteristic[index]->canRead()) {
    std::string value = sCharacteristic[index]->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }
  connectedDevice[index] = true;
  return true;
}


/**
 * 根据serverUUID连接server
 */
class MultiAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());

    //找到设备后，查找是否有我们需要的服务
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID1)) {

      Serial.println("Found device0");
      Device[0] = new BLEAdvertisedDevice(advertisedDevice);
      isFoundDevice[0]=true;
      readyToConnect[0] = true;

    }else if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID2))
    {

      Device[1] = new BLEAdvertisedDevice(advertisedDevice);
      isFoundDevice[1]=true;
      readyToConnect[1] = true;
    }else if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID3))
    {

      Device[2] = new BLEAdvertisedDevice(advertisedDevice);
      isFoundDevice[2]=true;
      readyToConnect[2] = true;
    }else{
      Serial.println("No service found");
      doScan = true;
    }
    if(isFoundDevice[0] && isFoundDevice[1] && isFoundDevice[2]){
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
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());

    //找到设备后，查找是否有我们需要的服务
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID1)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;

    }else{
      Serial.println("No service found");
      doScan = true;
    }
  } 
}; 

void ble_init(){
    
    Serial.println("Starting BLE application...");
    // xTaskCreate(&blinky, "blinky", 512,NULL,5,NULL );
    //从机初始化
    BLEDevice::init("helloESP32");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                                           CHARACTERISTIC_UUID,
                                           BLECharacteristic::PROPERTY_READ |
                                           BLECharacteristic::PROPERTY_WRITE
                                         );
  
    pCharacteristic->setValue("Hello World");
    pService->start();
    //得到广播对象
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);//设置是否响应广播
    pAdvertising->setMinPreferred(0x06); // 设置值越小，广播越频繁
    pAdvertising->setMinPreferred(0x12);// 设置值越大，广播越不频繁
    BLEDevice::startAdvertising();//开始广播
    Serial.println("Characteristic init OK");
  
    //主机初始化
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    #ifdef MULTI_BLE
    pBLEScan->setAdvertisedDeviceCallbacks(new MultiAdvertisedDeviceCallbacks());//设置广告回调
    #else
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());//设置广告回调
    #endif
    pBLEScan->setInterval(1349);//设置扫描间隔
    pBLEScan->setWindow(449);//设置扫描窗口
    pBLEScan->setActiveScan(true);//设置是否主动扫描
    pBLEScan->start(5, false);//开始扫描

    Serial.println("BLE init OK");

}
