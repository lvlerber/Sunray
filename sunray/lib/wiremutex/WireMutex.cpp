#include "WireMutex.h"

SemaphoreHandle_t wireMutex[I2C_BUS_COUNT];
int diepte =0;
bool takeWire(uint8_t bus_num, uint8_t timeout){
    if (!wireMutex[bus_num]) {
        wireMutex[bus_num] = xSemaphoreCreateRecursiveMutex();      
    } 
    diepte++;
    if (xSemaphoreTakeRecursive(wireMutex[bus_num],pdMS_TO_TICKS (timeout))) {
        // printf("%lu %d +++++++++++ %s\n",micros(),diepte ,pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));

        return true;
    } else {
        printf("uncaught exception of takeWire");
        return false;
    };
};
bool giveWire(uint8_t bus_num){
    if (!wireMutex[bus_num]) {
        wireMutex[bus_num] = xSemaphoreCreateRecursiveMutex();      
    } 
    diepte--;
    // printf("%lu %d ----------- %s\n",micros(),diepte , pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
    return xSemaphoreGiveRecursive(wireMutex[bus_num]);
}


