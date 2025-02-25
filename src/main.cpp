#include <Arduino.h>
#include "BLEDevice.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "ble.hpp"
#include "app.hpp"
//#include "BLEScan.h"
/*
任务列表：
1. 蓝牙
2.ADC采样
3.spi通讯
*/

#define ADC_QUEUE_LENGTH 10
#define SPI_QUEUE_LENGTH 10
#define BLE_QUEUE_LENGTH 10

QueueHandle_t xADCQueue;;
QueueHandle_t xSPIQueue;
QueueHandle_t xBLEQueue; 

void setup() {
  Serial.begin(115200);
  ble_init();
  xADCQueue  = xQueueCreate(ADC_QUEUE_LENGTH, sizeof(DataPacket));
  xSPIQueue  = xQueueCreate(SPI_QUEUE_LENGTH, sizeof(DataPacket));
  xBLEQueue  = xQueueCreate(BLE_QUEUE_LENGTH, sizeof(DataPacket));
  if (xADCQueue == NULL || xSPIQueue == NULL || xBLEQueue == NULL) {
    Serial.println("Failed to create queues.");
    return;
  }
  xTaskCreate(&ble_receive_task, "ble_task", 4096,NULL,5,NULL );
  xTaskCreate(&adc_task, "adc_task", 4096,NULL,5,NULL );
  // xTaskCreate(&spi_task, "spi_task", 4096,NULL,5,NULL );
  xTaskCreate(&BLESendTask, "BLESendTask", 4096,NULL,5,NULL );
} 


void loop() {
  
}