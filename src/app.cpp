#include "app.hpp"
#include<Arduino.h>
#include "ble.hpp"
#include "utils.hpp"


//是否启用多个BLE设备


#ifdef MULTI_BLE
//远程从机的UUID
static BLEUUID serviceUUID1("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID    char_uuid1("0000fff1-0000-1000-8000-00805f9b34fb");
static BLEUUID    char2_uuid1("0000fff2-0000-1000-8000-00805f9b34fb");

//远程从机的UUID
static BLEUUID serviceUUID2("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID    char_uuid2("0000fff1-0000-1000-8000-00805f9b34fb");
static BLEUUID    char2_uuid2("0000fff1-0000-1000-8000-00805f9b34fb");
//远程从机的UUID
static BLEUUID serviceUUID3("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID    char_uuid3("0000fff1-0000-1000-8000-00805f9b34fb");
static BLEUUID    char2_uuid3("0000fff1-0000-1000-8000-00805f9b34fb");

extern BLERemoteCharacteristic* sCharacteristic[3][2];
#endif


static BLEUUID service_uuid("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID    char_uuid("0000fff1-0000-1000-8000-00805f9b34fb");
extern BLERemoteCharacteristic* sRemoteCharacteristic;
extern BLERemoteCharacteristic* sRemoteCharacteristic2;
//队列
extern QueueHandle_t xADCQueue;
extern QueueHandle_t xSPIQueue;
extern QueueHandle_t xBLEQueue;

extern boolean doConnect;
extern boolean connected;
extern boolean doScan;

/*多链接标志位*/
extern boolean readyToConnect[3];
extern boolean connectedDevice[3];

//spi引用和定义


uint16_t spiCommand(SPIClass *spi, byte data);
static const int spiClk = 1000000;  // 1 MHz

//uninitialized pointers to SPI objects
SPIClass *fspi = NULL;

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
            uint32_t value = analogReadMilliVolts(3);
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
        vTaskDelay(5000 / portTICK_PERIOD_MS);//延时1s
        ADC_TASK_COPMLETE = true;
        // vTaskSuspend(NULL);
    }
}
/*
spi通讯读取传感器数据,添加到队列中
*/
void spi_task(void *pvParameter){
    fspi = new SPIClass(FSPI);
    fspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS); 
  
    //set up slave select pins as outputs as the Arduino API
    //doesn't handle automatically pulling SS low
    pinMode(fspi->pinSS(), OUTPUT);  //VSPI SS

    for(;;){
        DataPacket packet;
        packet.type = 1;//SPI

        uint16_t sum = 0;
        uint16_t max = 0;
        uint16_t min = 65535;
        uint16_t receivedData;

        for(int i = 0; i < 5; i++){
            receivedData = spiCommand(fspi,0x00);
            
            sum += receivedData;
            if(receivedData > max){
                max = receivedData;
            }
            if(receivedData < min){
                min = receivedData;
            }
        }
        uint16_t value = (sum - max - min) / 3; // 提取温度数据（12位）;然后转换为摄氏度;
        packet.data = value; //这里packet.data是uint32_t,温度值是float

        xQueueSend(xSPIQueue, &packet, portMAX_DELAY);
        vTaskDelay(5000 / portTICK_PERIOD_MS);//延时1s
        SPI_TASK_COPMLETE = true;
        //Serial.println(packet.data);
    }
}

/*
spi发送与接收函数。
*/
uint16_t spiCommand(SPIClass *spi, byte data) {

    uint16_t combinedData = 0;
  
    //use it as you would the regular arduino SPI API
    spi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(spi->pinSS(), LOW);  //pull SS slow to prep other end for transfer
    
    // 假设先发送高字节然后是低字节
    byte highByte = spi->transfer(data);  // 发送一个虚拟字节来获取数据
    byte lowByte = spi->transfer(data);   // 再次发送一个虚拟字节来获取第二个字节
  
    digitalWrite(spi->pinSS(), HIGH);  //pull ss high to signify end of data transfer
    spi->endTransaction();
  
    // 合并两个字节为一个16位整数
    combinedData = (highByte << 8) | lowByte;
    
    return combinedData;
  }
/*
蓝牙接收四个传感器的数据，
*/
#ifndef MULTI_BLE
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
            // Serial.println("BLE connected");
            if (sRemoteCharacteristic->canRead()) {
                uint8_t value = sRemoteCharacteristic->readUInt8();
                //添加到队列
                DataPacket packet;
                packet.type = 2;
                packet.data = value;
                xQueueSend(xBLEQueue, &packet, portMAX_DELAY);
                BLE_TASK_COPMLETE = true;//任务完成
            }
            // if (sRemoteCharacteristic2->canRead()) {
            //     uint8_t value = sRemoteCharacteristic2->readUInt8();
            //     //添加到队列
            //     DataPacket packet;
            //     packet.type = 2;
            //     packet.data = value;
            //     xQueueSend(xBLEQueue, &packet, portMAX_DELAY);
            // }

        }else if(doScan){
            BLEDevice::getScan()->start(0);
            doScan = false;//重新开始扫描
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);//延时1s
    }
}
#else
void ble_receive_task(void *pvParameter){
    for(;;){
        if(readyToConnect[0]){
            if (connectServerByUUID(0,serviceUUID1, char_uuid1, char2_uuid1)) {
                Serial.println("We are now connected to the BLE Server.");
            } else {
                Serial.println("We have failed to connect to the server; there is nothing more we will do.");
            }
            readyToConnect[0] = false;
        }
        if(connectedDevice[0]){
            if (sCharacteristic[0][0]->canRead() && sCharacteristic[0][1]->canRead()) {
                uint8_t value = sCharacteristic[0][0]->readUInt8();
                uint8_t value2 = sCharacteristic[0][1]->readUInt8();
                // Serial.printf("value was: %d, value2 was: %d\r\n", value, value2);
                /*第一个特征值是整数部分，第二个特征值是小数部分*/
                //拼数据
                float decimalPart = value2 / 256.0;
                float sensorData = value + decimalPart;
                
                //添加到队列
                DataPacket packet;
                packet.type = 0;
                // packet.data = value;
                packet.data = (uint32_t)(sensorData * 100);
                xQueueSend(xBLEQueue, &packet, portMAX_DELAY);
                BLE_TASK_COPMLETE = true;//任务完成
            }
        }

        if(readyToConnect[1]){
            if (connectServerByUUID(1,serviceUUID2, char_uuid2,char2_uuid2)) {
                Serial.println("We are now connected to the BLE Server.");
            } else {
                Serial.println("We have failed to connect to the server; there is nothing more we will do.");
            }
            readyToConnect[1] = false;
        }
        if(connectedDevice[1]){
            // if (sCharacteristic[1]->canRead()) {
            //     uint8_t value = sCharacteristic[1]->readUInt8();
            //     //添加到队列
            //     DataPacket packet;
            //     packet.type = 1;
            //     packet.data = value;
            //     xQueueSend(xBLEQueue, &packet, portMAX_DELAY);
            //     BLE_TASK_COPMLETE = true;//任务完成
            // }
        }

        if(readyToConnect[2]){
            if (connectServerByUUID(2,serviceUUID3, char_uuid3,char2_uuid3)) {
                Serial.println("We are now connected to the BLE Server.");
            } else {
                Serial.println("We have failed to connect to the server; there is nothing more we will do.");
            }
            readyToConnect[2] = false;
        }
        if(connectedDevice[2]){
            // if (sCharacteristic[2]->canRead()) {
            //     uint8_t value = sCharacteristic[2]->readUInt8();
            //     //添加到队列
            //     DataPacket packet;
            //     packet.type = 2;
            //     packet.data = value;
            //     xQueueSend(xBLEQueue, &packet, portMAX_DELAY);
            //     BLE_TASK_COPMLETE = true;//任务完成
            // }
        }

        if(connectedDevice[0] && connectedDevice[1] && connectedDevice[2]){
            Serial.println("All devices connected");
        }

        if(doScan){
            BLEDevice::getScan()->start(2,false);
            // doScan = false;//重新开始扫描
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);//延时1s
    }
}
#endif


extern BLECharacteristic *pCharacteristic;
extern BLECharacteristic *pCharacteristic2;
float distance = 0;
uint32_t last_data = 0;
extern BLEBeacon dataBeacon;

void BLESendTask(void *pvParameters) {
    float liu, tempCelsius;
    for(;;) {
      //检查ADC队列
      if (uxQueueMessagesWaiting(xADCQueue) > 0) {
        DataPacket packet;
        xQueueReceive(xADCQueue, &packet, 0);
        // Serial.print("ADC data: ");
        // Serial.println(packet.data);
        liu = adcConvert2Liu(packet.data); 
        Serial.printf("电流值:%.3f A\r\n", liu);
      }
      //检查SPI队列
      if (uxQueueMessagesWaiting(xSPIQueue) > 0) {
        DataPacket packet;
        xQueueReceive(xSPIQueue, &packet, 0);
        tempCelsius = ((packet.data >> 3) & 0x0FFF) * 0.25;
        Serial.print("温度数据: ");
        Serial.println(tempCelsius);
      }
      //检查BLE队列
      if (uxQueueMessagesWaiting(xBLEQueue) > 0) {
        DataPacket packet;
        xQueueReceive(xBLEQueue, &packet, 0);
        if(packet.type == 0){
            // if(packet.data != last_data){
                distance += packet.data ;
                Serial.print("loss data1: ");
                Serial.println(distance);
                last_data = packet.data;
                //将distance写入beacon major值
                setBeaconMajor((uint16_t)distance);
            // }
        }
      }
    //   Serial.printf("%f,%f,%f\n",liu,tempCelsius,distance);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

  
  