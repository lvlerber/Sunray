#include <Arduino.h>
#define TINY_GSM_MODEM_SIM800          // Modem is SIM800
#include <TinyGsmClient.h>
#include <src/ublox/ublox.h>
#include <config.h>
#include <confnvs.h>
#include <HTTPClient.h>

#define TEST_RING_RI_PIN            //Note will cancel the phone call test

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// #include "utilities.h"
// #include "SparkFun_Ublox_Arduino_Library.h"
// #include "gps.h"
// #include "ublox.h"
// #define GPS Serial1
//#define GPS_BAUDRATE 115200  defined in config.h
// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)

// Configure TinyGSM library

#define TINY_GSM_RX_BUFFER      1024   // Set RX buffer to 1Kb



#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(GSM, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(GSM);
#endif

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60        /* Time ESP32 will go to sleep (in seconds) */


TinyGsmClient client(modem);
// const int  port = 80;
const int  port = 2101; //for ntrip


extern  UBLOX gps; // defined in robot.cpp

void resetModem()
{
    // Need to reset after deep sleep, then Keep reset high
    pinMode(pinGsmReset, OUTPUT);
    digitalWrite(pinGsmReset, LOW);
    delay(100);
    digitalWrite(pinGsmReset, HIGH);
}

void turnOffNetlight()
{
    SerialMon.println("Turning off SIM800 Red LED...");
    modem.sendAT("+CNETLIGHT=0");
}

void turnOnNetlight()
{
    SerialMon.println("Turning on SIM800 Red LED...");
    modem.sendAT("+CNETLIGHT=1");
}

void startGPStest()
{
// Server details
// const char server[] = "vsh.pp.ua";
// const char resource[] = "/TinyGSM/logo.txt";
String server = NVS.getString(CONFIG_ITEMS[CONFIG_NTRIP_CLIENT_HOST].key);  // "212.204.120.33";
String resource = "/" +  NVS.getString(CONFIG_ITEMS[CONFIG_NTRIP_CLIENT_MOUNTPOINT].key);  //"/FLEPOSNRS32GREC";
int ntripport = NVS.getInt(CONFIG_ITEMS[CONFIG_NTRIP_CLIENT_PORT].key);
String ntripuser = NVS.getString(CONFIG_ITEMS[CONFIG_NTRIP_CLIENT_USERNAME].key); 
String ntrippass = NVS.getString(CONFIG_ITEMS[CONFIG_NTRIP_CLIENT_PASSWORD].key); 


// Your GPRS credentials (leave empty, if missing)
String apn      = NVS.getString(CONFIG_ITEMS[CONFIG_GSM_APN].key); //"web.be"; // Your APN
String gprsUser = NVS.getString(CONFIG_ITEMS[CONFIG_GSM_GPRS_USER].key);  //"web"; // User
String gprsPass = NVS.getString(CONFIG_ITEMS[CONFIG_GSM_GPRS_PASS].key);  //"web"; // Password
String simPIN   = NVS.getString(CONFIG_ITEMS[CONFIG_GSM_SIMPIN].key); // SIM card PIN code, if any

    // Start power management -- not present in hardware

    // Some start operations
    resetModem();

    // Set GSM module baud rate and UART pins
    // MODEM.begin(115200, SERIAL_8N1, 16, 17);
        GSM.begin(115200, SERIAL_8N1, pinGsmRx, pinGsmTx);


gps.begin(GPS,GPS_BAUDRATE);   
GPS.setPins(pinGpsRx, pinGpsTx);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println("Initializing modem...");
    modem.restart();

    // Turn off network status lights to reduce current consumption
    turnOffNetlight();


    // The status light cannot be turned off, only physically removed
    //turnOffStatuslight();

    // Or, use modem.init() if you don't need the complete restart
    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem: ");
    SerialMon.println(modemInfo);
 
    // Unlock your SIM card with a PIN if needed
    if (strlen(simPIN.c_str()) && modem.getSimStatus() != 3 ) {
        modem.simUnlock(simPIN.c_str());
    }

    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork(240000L)) {
        SerialMon.println(" fail");
        delay(1000);
        // return; temporarily disabled
    }
    if (modem.isNetworkConnected () ) {
    SerialMon.println(" OK");
        turnOnNetlight();
  SerialMon.print("imei: ");
    SerialMon.println(modem.getIMEI());
   SerialMon.print("operator: ");
    SerialMon.println(modem.getOperator());
    SerialMon.print("simlstatus: ");
    SerialMon.println(modem.getSimStatus());
    SerialMon.print("sigqual: ");
    SerialMon.println(modem.getSignalQuality());
    SerialMon.print("regstatus: ");
    SerialMon.println(modem.getRegistrationStatus());

    // When the network connection is successful, turn on the indicator
    // digitalWrite(LED_GPIO, LED_ON);

    if (modem.isNetworkConnected()) {
        SerialMon.println("Network connected");
    }

    SerialMon.print(F("Connecting to APN: "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn.c_str(), gprsUser.c_str(), gprsPass.c_str())) {
        SerialMon.println(" fail");
        delay(1000);
        return;
    }
    SerialMon.println(" OK");

    SerialMon.print("Connecting to ");
    SerialMon.print(server);
    if (!client.connect(server.c_str(), port)) {
        SerialMon.println(" fail");
        delay(1000);
        return;
    }
    SerialMon.println(" OK");

    // Make a HTTP GET request:

    SerialMon.println("Performing HTTP GET request...");
    client.print(String("GET ") + resource + " HTTP/1.1\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print(String("Authorization: ") +"Basic MTA2MDk1YTAwMjo3ODk2Mw==" + "\r\n");
    client.print(String("Ntrip-GGA: ")+"$GPGGA,182136,5053.85,N,00330.48,E,1,10,1,42.0,M,46.0,M,5,0*60" + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.println();

    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 100000L) {
        // Print available data
        while (client.available()) {
            char c = client.read();
            SerialMon.print(c);
            GPS.print(c);
            timeout = millis();
            gps.run();
        }
    }
    
    SerialMon.println();
timeout = millis() +  100000L;
    for (int i=1; i<2; i++) {
        // Print available data
        
            SerialMon.println(gps.accuracy);
            SerialMon.println(gps.dgpsAge);            
            SerialMon.println(gps.dgpsChecksumErrorCounter);                  
            SerialMon.println(gps.dgpsPacketCounter);                  
            SerialMon.println(gps.hAccuracy);             
            SerialMon.println(gps.lat);             
            SerialMon.println(gps.lon);             
            SerialMon.println(gps.solution);             
            SerialMon.println(gps.solutionAvail);             
            SerialMon.println(gps.iTOW);             
    }
    // Shutdown
    client.stop();
    SerialMon.println(F("Server disconnected"));

    modem.gprsDisconnect();
    SerialMon.println(F("GPRS disconnected"));


    // DTR is used to wake up the sleeping Modem
    // DTR is used to wake up the sleeping Modem
    // DTR is used to wake up the sleeping Modem
#ifdef MODEM_DTR
    bool res;

    modem.sleepEnable();

    delay(100);

    // test modem response , res == 0 , modem is sleep
    res = modem.testAT();
    Serial.print("SIM800 Test AT result -> ");
    Serial.println(res);

    delay(1000);

    Serial.println("Use DTR Pin Wakeup");
    pinMode(MODEM_DTR, OUTPUT);
    //Set DTR Pin low , wakeup modem .
    digitalWrite(MODEM_DTR, LOW);


    // test modem response , res == 1 , modem is wakeup
    res = modem.testAT();
    Serial.print("SIM800 Test AT result -> ");
    Serial.println(res);

#endif


#ifdef TEST_RING_RI_PIN
#ifdef MODEM_RI
    // Swap the audio channels
    MODEM.print("AT+CHFA=1\r\n");
    delay(2);

    //Set ringer sound level
    MODEM.print("AT+CRSL=100\r\n");
    delay(2);

    //Set loud speaker volume level
    MODEM.print("AT+CLVL=100\r\n");
    delay(2);

    // Calling line identification presentation
    MODEM.print("AT+CLIP=1\r\n");
    delay(2);

    //Set RI Pin input
    pinMode(MODEM_RI, INPUT);

    Serial.println("Wait for call in");
    //When is no calling ,RI pin is high level
    while (digitalRead(MODEM_RI)) {
        Serial.print('.');
        delay(500);
    }
    Serial.println("call in ");

    //Wait 10 seconds for the bell to ring
    delay(10000);

    //Accept call
    MODEM.println("ATA");


    delay(10000);

    // Wait ten seconds, then hang up the call
    MODEM.println("ATH");
#endif  //MODEM_RI
#endif  //TEST_RING_RI_PIN

    // Make the LED blink three times before going to sleep
    int i = 3;
    while (i--) {
        // digitalWrite(LED_GPIO, LED_ON);
        modem.sendAT("+SPWM=0,1000,80");
        delay(500);
        // digitalWrite(LED_GPIO, LED_OFF);
        modem.sendAT("+SPWM=0,1000,0");
        delay(500);
    }

    //After all off
    modem.poweroff();

    SerialMon.println(F("Poweroff"));

    // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    // esp_deep_sleep_start();

    /*
    The sleep current using AXP192 power management is about 500uA,
    and the IP5306 consumes about 1mA
    */
    } else{
        Serial.println(server);
        Serial.println(resource);
        Serial.println(ntripuser);
        Serial.println(ntrippass);                        
        HTTPClient http;
        String url="http://";
        url += server;
        url += ":";
        url += ntripport;
        url += resource;  // includes /
        Serial.println("url:"+url);
        http.begin(url);
        http.setAuthorizationType("Basic");
        http.setAuthorization(ntripuser.c_str(),ntrippass.c_str());
        http.addHeader("Ntrip-Version: ", "Ntrip/2.0");
        http.addHeader("Ntrip-GGA: ", "$GPGGA,182136,5053.85,N,00330.48,E,1,10,1,42.0,M,46.0,M,5,0*60");
        http.GET();
            SerialMon.println("Performing HTTP GET request...");
SerialMon.print("connected" );
SerialMon.println(http.connected());
SerialMon.print("aantal bytes available");
SerialMon.println(http.getStream().available());
    unsigned long timeout = millis();
    while (http.connected() && millis() - timeout < 100000L) {
        // Print available data
        while (http.getStream().available()) {
            char c = http.getStream().read();
            SerialMon.print(c);
            GPS.print(c);
            timeout = millis();
            gps.run();
        }
    }
    

    }
}
