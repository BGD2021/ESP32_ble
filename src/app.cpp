#include "app.hpp"
#include<Arduino.h>
#include "ble.hpp"

extern BLERemoteCharacteristic* pRemoteCharacteristic;
extern BLERemoteCharacteristic* pRemoteCharacteristic2;
/*
ADC采样霍尔传感器添加到队列中
*/
void adc_task(void *pvParameter){
    analogReadResolution(12);
    for(;;){
        DataPacket packet;
        packet.type = 0;
        packet.data = analogReadMilliVolts(34);//读取引脚34的电压值

        vTaskDelay(1000 / portTICK_PERIOD_MS);//延时1s
    }
}
/*
spi通讯读取传感器数据,添加到队列中
*/
void spi_task(void *pvParameter){

}

/*
蓝牙接收四个传感器的数据，
*/
void ble_receive_task(void *pvParameter){
    for(;;){
        // //初始化后开始连接
        // if (doConnect == true) {
        //     if (connectToServer()) {
        //     Serial.println("We are now connected to the BLE Server.");
        //     } else {
        //     Serial.println("We have failed to connect to the server; there is nothing more we will do.");
        //     }
        //     doConnect = false;
        // }

        // //连接后开始读写
        // if (connected) {
        //     uint8_t newValue = 0x18;  // 16进制的18
        //     Serial.println("Setting new characteristic value to 0x18");
            
        
        //     pRemoteCharacteristic->writeValue(&newValue, 1);


        //     if (pRemoteCharacteristic->canRead()) {
        //     std::string value = pRemoteCharacteristic->readValue();
        //     Serial.print("The characteristic1 value is: ");
        //     printHex(value);
        //     }
        //     if (pRemoteCharacteristic2->canRead()) {
        //     std::string value = pRemoteCharacteristic2->readValue();
        //     Serial.print("The characteristic2 value is: ");
        //     printHex(value);
        //     }
        // } else if (doScan) {//如果没有连接，继续扫描
        //     BLEDevice::getScan()->start(0);
        // }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);//延时1s
    }
}