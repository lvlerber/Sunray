// #include "esp32-expanders-hal-custom.h"
#include <Arduino.h>
#include <config.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
// #include <ESPAsyncWebServer.h>
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

// Required do define ResourceNodes
#include <ResourceNode.hpp>

// Easier access to the classes of the server
using namespace httpsserver;

void scanner();

HTTPServer myServer = HTTPServer(80);
bool WebSerialActive = false;
extern String cmd;
extern Battery battery;
extern AmBatteryDriver batteryDriver;

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

void createDir(fs::FS &fs, const char *path)
{
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path))
  {
    Serial.println("Dir created");
  }
  else
  {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path)
{
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path))
  {
    Serial.println("Dir removed");
  }
  else
  {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2))
  {
    Serial.println("File renamed");
  }
  else
  {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path)
{
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file)
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len)
    {
      size_t toRead = len;
      if (toRead > 512)
      {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  }
  else
  {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++)
  {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}
void task_WebConsoleInputHandler(void *args);

void WebConsoleInputHandler(uint8_t *data, size_t len)
{
  cmd=String(data,len);
  Serial.println(cmd);
  delay(10);
  xTaskCreate(task_WebConsoleInputHandler, "task_webconsoleinput", 4096, NULL, tskIDLE_PRIORITY, NULL);
};
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

/*   uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  listDir(SD, "/", 0);
  createDir(SD, "/mydir");
  listDir(SD, "/", 0);
  removeDir(SD, "/mydir");
  listDir(SD, "/", 2);
  writeFile(SD, "/hello.txt", "Hello ");
  appendFile(SD, "/hello.txt", "World!\n");
  readFile(SD, "/hello.txt");
  deleteFile(SD, "/foo.txt");
  renameFile(SD, "/hello.txt", "/foo.txt");
  readFile(SD, "/foo.txt");
  testFileIO(SD, "/test.txt");
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024)); */
  /**
   * Connect to Wifi
   */

  RoamingWiFi.start();
  ArduinoOTA.begin();
  WebSerial.msgCallback(WebConsoleInputHandler);
  WebSerial.begin(&myServer);


  webServices.begin(&myServer);
  // myServer.start();  is now included in the previous line
  WebSerialActive = true;
  bluetooth.run();
 
  void startGPStest();
  digitalWrite(pinBatterySwitch,HIGH);
  startGPStest();
 
  batteryDriver.begin();
  // pin 15 is pulled high by the bootstrapping esp32.  After boot it is free to use, but you need to reset it manually  
  analogWrite(pinMotorMowPWM,0);
 battery.begin();  
}

void loop()
{
  // Check for over the air update request and (if present) flash it

  ArduinoOTA.handle();

  delay(1000);
  // printf("message");
   battery.run();
   batteryDriver.run();
}

// simulate Ardumower answer (only for BLE testing) 
int simPacketCounter;
extern String cmdResponse;
String processCmdBluetooth(String req){
    String resp;
  simPacketCounter++;
  if (req.startsWith("AT+V")){
    resp = "V,Ardumower Sunray,1.0.219,0,78,0x56\n";
  }
  if (req.startsWith("AT+P")){
    resp = "P,0x50\n";  
  }
  if (req.startsWith("AT+M")){        
    resp = "M,0x4d\n";
  }
  if (req.startsWith("AT+S")){        
    if (simPacketCounter % 2 == 0){
      resp = "S,28.60,15.15,-10.24,2.02,2,2,0,0.25,0,15.70,-11.39,0.02,49,-0.05,48,-971195,0x92\n";
    } else {
      resp = "S,27.60,15.15,-10.24,2.02,2,2,0,0.25,0,15.70,-11.39,0.02,49,-0.05,48,-971195,0x91\n";
    }        
  }     
  if (! req.startsWith("AT+")){
    WebConsoleInputHandler((uint8_t *) req.c_str(), strlen(req.c_str()));
    resp="submitted";
  }
  Serial.println("respnonse : " + resp);
  return resp;   
}