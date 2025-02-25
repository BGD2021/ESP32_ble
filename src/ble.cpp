#include <Arduino.h>
#include "BLEDevice.h"
#include "app.hpp"
//#include "BLEScan.h"

//从机的UUID
static BLEUUID serviceUUID("0000fff0-0000-1000-8000-00805f9b34fb");

static BLEUUID    charUUID("0000fff1-0000-1000-8000-00805f9b34fb");
static BLEUUID   charUUID2("0000fff2-0000-1000-8000-00805f9b34fb");
//连接标志位
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
//特征值和设备
BLERemoteCharacteristic* pRemoteCharacteristic;
BLERemoteCharacteristic* pRemoteCharacteristic2;
static BLEAdvertisedDevice* myDevice;
//主机的UUID
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
    connected = false;
    Serial.println("onDisconnect");
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
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    // 防止连接失败，程序跑飞
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    //连接特征值
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    pRemoteCharacteristic2 = pRemoteService->getCharacteristic(charUUID2);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID2.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    //首次读取特征值
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

/**
 * 根据serverUUID连接server
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    //找到设备后，查找是否有我们需要的服务
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
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
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                           CHARACTERISTIC_UUID,
                                           BLECharacteristic::PROPERTY_READ |
                                           BLECharacteristic::PROPERTY_WRITE
                                         );
  
    pCharacteristic->setValue("Hello World says Neil");
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
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);//设置扫描间隔
    pBLEScan->setWindow(449);//设置扫描窗口
    pBLEScan->setActiveScan(true);//设置是否主动扫描
    pBLEScan->start(5, false);//开始扫描

    Serial.println("BLE init OK");

}

void ble_task(void *pvParameter){
    for(;;){
        //初始化后开始连接
        if (doConnect == true) {
            if (connectToServer()) {
            Serial.println("We are now connected to the BLE Server.");
            } else {
            Serial.println("We have failed to connect to the server; there is nothing more we will do.");
            }
            doConnect = false;
        }

        //连接后开始读写
        if (connected) {
            uint8_t newValue = 0x18;  // 16进制的18
            Serial.println("Setting new characteristic value to 0x18");
            
        
            pRemoteCharacteristic->writeValue(&newValue, 1);


            if (pRemoteCharacteristic->canRead()) {
            std::string value = pRemoteCharacteristic->readValue();
            Serial.print("The characteristic1 value is: ");
            printHex(value);
            }
            if (pRemoteCharacteristic2->canRead()) {
            std::string value = pRemoteCharacteristic2->readValue();
            Serial.print("The characteristic2 value is: ");
            printHex(value);
            }
        } else if (doScan) {//如果没有连接，继续扫描
            BLEDevice::getScan()->start(0);
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);//延时1s
    }
}

void BLESendTask(void *pvParameters) {
  for(;;) {
    //检查ADC队列
    if (uxQueueMessagesWaiting(xADCQueue) > 0) {
      DataPacket packet;
      xQueueReceive(xADCQueue, &packet, 0);
      Serial.print("ADC data: ");
      Serial.println(packet.data);
    }
    //检查SPI队列
    if (uxQueueMessagesWaiting(xSPIQueue) > 0) {
      DataPacket packet;
      xQueueReceive(xSPIQueue, &packet, 0);
      Serial.print("SPI data: ");
      Serial.println(packet.data);
    }
    //检查BLE队列
    if (uxQueueMessagesWaiting(xBLEQueue) > 0) {
      DataPacket packet;
      xQueueReceive(xBLEQueue, &packet, 0);
      Serial.print("BLE data: ");
      Serial.println(packet.data);
    }

    // Serial.println("BLESendTask");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}


