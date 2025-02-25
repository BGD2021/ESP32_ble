#ifndef APP_HPP
#define APP_HPP

#include <Arduino.h>

// 数据包结构体
typedef struct {
    uint8_t type;     // 0: ADC, 1: SPI, 2: BLE
    uint32_t data;    // 示例数据，具体结构根据实际情况定义
} DataPacket;

// 任务函数声明
void adc_task(void *pvParameter);
void spi_task(void *pvParameter);
void ble_receive_task(void *pvParameter);

#endif // APP_HPP
