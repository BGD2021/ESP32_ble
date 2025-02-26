#include "app.hpp"
#include<Arduino.h>
#include "ble.hpp"

extern BLERemoteCharacteristic* sRemoteCharacteristic;
extern BLERemoteCharacteristic* sRemoteCharacteristic2;
//队列
extern QueueHandle_t xADCQueue;
extern QueueHandle_t xSPIQueue;
extern QueueHandle_t xBLEQueue;

extern boolean doConnect;
extern boolean connected;
extern boolean doScan;

//任务完成标志
bool ADC_TASK_COPMLETE = false;
bool SPI_TASK_COPMLETE = false;
bool BLE_TASK_COPMLETE = false;
/*
ADC采样霍尔传感器添加到队列中
*/
void adc_task(void *pvParameter){
    analogReadResolution(12);
    for(;;){
        DataPacket packet;
        packet.type = 0;
        // packet.data = analogReadMilliVolts(1);//读取引脚34的电压值
        //进行五次采样去掉最高最低值，取平均值
        uint32_t sum = 0;
        uint32_t max = 0;
        uint32_t min = 4096;
        for(int i = 0; i < 5; i++){
            uint32_t value = analogRead(1);
            sum += value;
            if(value > max){
                max = value;
            }
            if(value < min){
                min = value;
            }
        }
        packet.data = (sum - max - min) / 3;
        xQueueSend(xADCQueue, &packet, portMAX_DELAY);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);//延时1s
        ADC_TASK_COPMLETE = true;
        vTaskSuspend(NULL);
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
        //初始化后开始连接
        if (doConnect == true) {
            if (connectToServer()) {
                Serial.println("We are now connected to the BLE Server.");
            } else {
                Serial.println("We have failed to connect to the server; there is nothing more we will do.");
            }
            doConnect = false;
        }
        if(connected){
            if (sRemoteCharacteristic->canRead()) {
                uint8_t value = sRemoteCharacteristic->readUInt8();
                //添加到队列
                DataPacket packet;
                packet.type = 2;
                packet.data = value;
                xQueueSend(xBLEQueue, &packet, portMAX_DELAY);
                BLE_TASK_COPMLETE = true;//任务完成
            }
            if (sRemoteCharacteristic2->canRead()) {
                uint8_t value = sRemoteCharacteristic2->readUInt8();
                //添加到队列
                DataPacket packet;
                packet.type = 2;
                packet.data = value;
                xQueueSend(xBLEQueue, &packet, portMAX_DELAY);
            }
        }else if(doScan){
            BLEDevice::getScan()->start(0);
            doScan = false;//重新开始扫描
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);//延时1s
    }
}

extern BLECharacteristic *pCharacteristic;
extern BLECharacteristic *pCharacteristic2;


void BLESendTask(void *pvParameters) {
    for(;;) {
      //检查ADC队列
      if (uxQueueMessagesWaiting(xADCQueue) > 0) {
        DataPacket packet;
        xQueueReceive(xADCQueue, &packet, 0);
        Serial.print("ADC data: ");
        Serial.println(packet.data);
        //将hex数据转换为字符串
        char str[10];
        sprintf(str, "%d", packet.data);
        pCharacteristic->setValue(str);
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
        char str[10];
        sprintf(str, "%d", packet.data);
        pCharacteristic->setValue(str);
      }
  
      // Serial.println("BLESendTask");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
  
  