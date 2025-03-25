#include "ble2.hpp"
#include "config.h"
#include "utils.hpp"


// BLEUUID serviceUUID[] = {
//     BLEUUID("0000fff0-0000-1000-8000-00805f9b34fb"),
//     BLEUUID("fffffff0-0000-1000-8000-00805f9b34fb"),
//     BLEUUID("fffffff0-0000-1000-8000-00805f9b34fb")
// }; 

// BLEUUID charUUID[] = {
//     BLEUUID("0000fff1-0000-1000-8000-00805f9b34fb"),
//     BLEUUID("fffffff1-0000-1000-8000-00805f9b34fb"),
//     BLEUUID("fffffff1-0000-1000-8000-00805f9b34fb")
// };


class bleSlave{
    private:
        ble_device_info info;
        bool isConnect;
        BLERemoteCharacteristic* Characteristic[maxDevice];
        BLEAdvertisedDevice* myDevice;
        BLERemoteService* pRemoteService;

        class bleClientCallback : public BLEClientCallbacks {
            void onConnect(BLEClient* pclient) {

            }
          
            void onDisconnect(BLEClient* pclient) {
              //TODO:对每个设备都加入断开连接的处理

            }
        };

    public:
        //构造函数
        bleSlave(BLEAdvertisedDevice* Device, ble_device_info info){
            this->info = info;
            this->isConnect = false;
            this->myDevice = Device;
        }
        //连接函数
        boolean bleConnect(){
            Serial.print("Forming a connection to ");
            Serial.println(myDevice->getAddress().toString().c_str());
            BLEClient*  pClient  = BLEDevice::createClient();
            Serial.println(" - Created client");

            pClient->setClientCallbacks(new bleClientCallback());

            // Connect to the remove BLE Server.
            if (!pClient->connect(myDevice)) {  // 检查连接是否成功
                Serial.println(" - Failed to connect to server");
                return false;
            }
            Serial.println(" - Connected to server");

            // Obtain a reference to the service we are after in the remote BLE server.
            pRemoteService = pClient->getService(info.serviceUUID);
            // 防止连接失败，程序跑飞
            if (pRemoteService == nullptr) {
                Serial.print("Failed to find our service UUID: ");
                Serial.println(info.serviceUUID.toString().c_str());
                pClient->disconnect();
                return false;
            }

            for(int i;i<maxChar;i++){
                Characteristic[i] = pRemoteService->getCharacteristic(info.charUUID[i]);
                if (Characteristic[i] == nullptr) {
                    Serial.print("Failed to find our characteristic UUID: ");
                    Serial.println(info.charUUID[i].toString().c_str());
                    pClient->disconnect();
                    return false;
                }else{
                    break;//如果没有更多的特征值那么就取消连接
                }
                Serial.printf(" - Found our characteristic[%d]\r\n",i);
            }
            isConnect = true;
        }
        //读取特征值
        uint8_t readCharacteristic(uint8_t index){
            if(Characteristic[index]->canRead()) {
                uint8_t value = Characteristic[index]->readUInt8();
                return value;
            }
        }
};


class ScanManager{
    private:
        BLEScan* bleScanner;
        bool doscan;



        class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
                void onResult(BLEAdvertisedDevice advertisedDevice) {
                    //找到设备后，查找是否有我们需要的服务
                    if (advertisedDevice.haveServiceUUID() && isFoundDevice(advertisedDevice)) {
                        //放入待连接队列

                    }else{
                    }
                } 
          }; 

};