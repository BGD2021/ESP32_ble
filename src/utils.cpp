
#include "utils.hpp"
#include "config.h"

bool isFoundDevice(BLEAdvertisedDevice advertisedDevice){
    uint8_t index = 0;
    for(index=0;index<maxDevice;index++){
        if (advertisedDevice.isAdvertisingService(serviceUUID[index])){
            return true;
        }                
    }
    return false;
}

float adcConvert2Liu(uint16_t adcValue){
    float liu;
    liu = ((float)adcValue * 3 / 40) - 125;
    if(liu < 0){
      liu = 0;
    //   Serial.println("电流值小于0,请检查传感器连接");
    }  
    return liu;
}

