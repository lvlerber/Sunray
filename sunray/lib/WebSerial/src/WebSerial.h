#ifndef WebSerial_h
#define WebSerial_h

#include "Arduino.h"
#include "stdlib_noniso.h"
#include <functional>
// Inlcudes for setting up the server
#include <HTTPSServer.hpp>

// Define the certificate data for the server (Certificate and private key)
#include <SSLCert.hpp>

// Includes to define request handler callbacks
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

// Required do define ResourceNodes
#include <ResourceNode.hpp>
#include <WebsocketHandler.hpp>
// Easier access to the classes of the server
using namespace httpsserver;
#include <WebsocketHandler.hpp>


#if defined(ESP8266)
    #define HARDWARE "ESP8266"
    #include "ESP8266WiFi.h"
    #include "ESPAsyncTCP.h"
    #include "ESPAsyncWebServer.h"
#elif defined(ESP32)
    #define HARDWARE "ESP32"
    // #include "AsyncTCP.h"
    // #include "ESPAsyncWebServer.h"
    #include "WiFi.h"
#endif

typedef std::function<void(uint8_t *data, size_t len)> RecvMsgHandler;

// class WebSerialHandler : public WebsocketHandler
// {
// public:
//     // This method is called by the webserver to instantiate a new handler for each
//     // client that connects to the websocket endpoint
//     static WebsocketHandler *create();
//     // This method is called when a message arrives
//     void onMessage(WebsocketInputStreambuf *input);
//     // Handler function on connection close
//     void onClose();
// };

// Uncomment to enable webserial debug mode
// #define WEBSERIAL_DEBUG 0

class WebSerialClass{

public:
    void begin(HTTPServer *server, const char* url = "/webserial");

    void msgCallback(RecvMsgHandler _recv);

    // Print

    void print(String m = "");

    void print(const char *m);

    void print(char *m);

    void print(int m);

    void print(uint8_t m);

    void print(uint16_t m);

    void print(uint32_t m);

    void print(double m);

    void print(float m);

    void print(const uint8_t *buffer, size_t size);


    // Print with New Line

    void println(String m = "");

    void println(const char *m);

    void println(char *m);

    void println(int m);

    void println(uint8_t m);

    void println(uint16_t m);

    void println(uint32_t m);

    void println(float m);

    void println(double m);

    void textAll(uint8_t *m, size_t len);    
    void textAll(String m);
    void textAll(char *m);


private:
    HTTPServer *_server;
    // AsyncWebSocket *_ws;
    RecvMsgHandler _RecvFunc = NULL;
    
    #if defined(WEBSERIAL_DEBUG)
        void DEBUG_WEB_SERIAL(const char* message);
    #endif
};

extern WebSerialClass WebSerial;
int multi_writefn(void *cookie, const char *data, int n) ;
extern bool WebSerialActive;
#endif