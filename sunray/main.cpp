#include <Arduino.h>
#include <config.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WebServices.h>
#include <confnvs.h>
#include "RoamingWifi.h"
#include <ArduinoOTA.h>
#include <WebSerial.h>
#include <pinman.h>
#include "battery.h"
#include "src/driver/AmRobotDriver.h"
#include <Wire.h>
#include "bluetooth.h"
// Inlcudes for setting up the server
#include <HTTPSServer.hpp>

// Define the certificate data for the server (Certificate and private key)
#include <SSLCert.hpp>

// Includes to define request handler callbacks
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <i2c.h>
#include <version.h>

// Required do define ResourceNodes
// #include <ResourceNode.hpp>
#include "robot.h"

// Easier access to the classes of the server
using namespace httpsserver;

HTTPServer myServer = HTTPServer(80);
bool WebSerialActive = false;
extern String cmd;
extern Battery battery;
extern AmBatteryDriver batteryDriver;
  bool ju=false;
void task_ota(void *args)
{
  ArduinoOTA.begin();
  while (1)
  {
    // Check for over the air update request and (if present) flash it
    ArduinoOTA.handle();
    delay(5000);
  }
};
void task_WebConsoleInputHandler(void *args);

void WebConsoleInputHandler(uint8_t *data, size_t len)
{
  cmd=String(data,len);
  Serial.println(cmd);
  delay(10);
  xTaskCreate(task_WebConsoleInputHandler, "task_webconsoleinput", 4096, NULL, tskIDLE_PRIORITY, NULL);
};
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.path(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void setup()
{

  _GLOBAL_REENT->_stdout = funopen(NULL, NULL, &multi_writefn, NULL, NULL);
  _GLOBAL_REENT->_stderr = funopen(NULL, NULL, &multi_writefn, NULL, NULL);
  setbuf(_GLOBAL_REENT->_stdout, NULL);
  setbuf(_GLOBAL_REENT->_stderr, NULL);
  // Also redirect stdout/stderr of main task
  stdout = _GLOBAL_REENT->_stdout;
  stderr = _GLOBAL_REENT->_stderr;
  ConfNvs confNvs;
  confNvs.begin();
  Serial.begin(CONSOLE_BAUDRATE);  
  GPS.begin(GPS_BAUDRATE);
  GPS.setPins(pinGpsRx, pinGpsTx);
  pinMan.begin();
 

  // The select pin for the SD card is 0, the others are standard
  if (!SD.begin(0,SPI,4000000U,"/sd",10,false))
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
  }
  listDir(SD, "/", 0);
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.println(VERSION);
  /**
   * Connect to Wifi
   */

  RoamingWiFi.start();
  xTaskCreate(task_ota, "task_ArduinOTA", 4096, NULL, tskIDLE_PRIORITY, NULL);
  WebSerial.msgCallback(WebConsoleInputHandler);
  WebSerial.begin(&myServer);


  webServices.begin(&myServer);
  // myServer.start();  is now included in the previous line
  WebSerialActive = true;
  bluetooth.run();
 
  void startGPStest();
  digitalWrite(pinBatterySwitch,HIGH);
  // startGPStest();
 delay(50);
  // batteryDriver.begin();
  // pin 15 is pulled high by the bootstrapping esp32.  After boot it is free to use, but you need to reset it manually  
  analogWrite(pinMotorMowPWM,0);
//  battery.begin();  
Wire.begin(-1,-1,400000);
  I2CScanner();
// delay(30000);
  while (!ju)
  {
    // CONSOLE.println(100000);
    // Wire.begin(-1, -1, 100000);
    // I2CScanner();

    // delay(100);
    // CONSOLE.println(400000);    
    // Wire.begin(-1, -1, 400000);
    // I2CScanner();
    // delay(100);
    //     CONSOLE.println(1000000);
    //       Wire.begin(-1, -1, 1000000);
    // I2CScanner();
    delay(10000);
    }
  ju=false;
  start();

}

void loop()
{
  
  run();

  // delay(1000);
  // printf("message");
  //  battery.run();
  //  batteryDriver.run();
}

