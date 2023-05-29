/** NimBLE_Server Demo stripped and adapted to fit in ArduMower transformed to ESP32Mower
 *
 *
 *  Created: on March 22 2023
 *      Author: Lieven Vanlerberghe
 *
*/

#include <NimBLEDevice.h>
#include "bluetooth.h"
#include <confnvs.h>
#include "config.h"
String processCmdBluetooth(String cmd);
#ifndef DEBUG_NIMBLE
#define DEBUG_NIMBLE 0
#endif
#define BLE_MTU 20
static NimBLEServer* pServer;

void nimbleSendResponse(NimBLECharacteristic* pCharacteristic,uint8_t* data, int len){
    int offset=0;
    while (offset < len){
        size_t lentosend = min(len-offset,BLE_MTU);
        pCharacteristic->setValue(data+offset,lentosend);
        pCharacteristic->notify();
        offset += BLE_MTU;
    }
}

void nimbleSendResponse(NimBLECharacteristic* pCharacteristic,String resp){
    nimbleSendResponse(pCharacteristic,(uint8_t*)resp.c_str(),resp.length());
}

/** Handler class for characteristic actions */
class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic){
        #if DEBUG_NIMBLE
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onRead(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
        #endif
    };

    void onWrite(NimBLECharacteristic* pCharacteristic) {
        #if DEBUG_NIMBLE
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onWrite(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
        #endif
        nimbleSendResponse(pCharacteristic,processCmdBluetooth(pCharacteristic->getValue().c_str()));
    };
    /** Called before notification or indication is sent,
     *  the value can be changed here before sending if desired.
     */
    void onNotify(NimBLECharacteristic* pCharacteristic) {
        // Serial.println("Sending notification to clients");
    };


    void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
        #if DEBUG_NIMBLE        
        String str = "Client ID: ";
        str += desc->conn_handle;
        str += " Address: ";
        str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
        if(subValue == 0) {
            str += " Unsubscribed to ";
        }else if(subValue == 1) {
            str += " Subscribed to notfications for ";
        } else if(subValue == 2) {
            str += " Subscribed to indications for ";
        } else if(subValue == 3) {
            str += " Subscribed to notifications and indications for ";
        }
        str += std::string(pCharacteristic->getUUID()).c_str();

        Serial.println(str);
        #endif
    };
};

/** Define callback instances globally to use for multiple Charateristics \ Descriptors */
// static DescriptorCallbacks dscCallbacks;
static CharacteristicCallbacks chrCallbacks;


void Bluetooth::run() {
    Serial.begin(CONSOLE_BAUDRATE);
    Serial.println("Starting NimBLE Server");

    /** sets device name */
    NimBLEDevice::init(NVS.getString(CONFIG_ITEMS[CONFIG_BLUETOOTH_DEVICE_NAME].key).c_str(

    ));

    /** Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
#else
    NimBLEDevice::setPower(9); /** +9db */
#endif

    pServer = NimBLEDevice::createServer();

#define SERVICE_UUID        "0000FFE0-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "0000FFE1-0000-1000-8000-00805F9B34FB"
    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    NimBLECharacteristic* pdefCharacteristic = pService->createCharacteristic(
                                               CHARACTERISTIC_UUID,
                                               NIMBLE_PROPERTY::READ |
                                               NIMBLE_PROPERTY::WRITE |
                                               NIMBLE_PROPERTY::WRITE_NR |  // client can write without our acknowledge (no-response)
                                               NIMBLE_PROPERTY::NOTIFY 
                               /** Require a secure connection for read and write access */
                                            //    NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
                                            //    NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
                                              );

    pdefCharacteristic->setValue("");
    pdefCharacteristic->setCallbacks(&chrCallbacks);

    /** Start the services when finished creating all Characteristics and Descriptors */
    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    /** Add the services to the advertisment data **/
    pAdvertising->addServiceUUID(pService->getUUID());
    /** If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("Advertising Started");
}
Bluetooth bluetooth;