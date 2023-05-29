#pragma once

#include "Arduino.h"
#include "stdlib_noniso.h"
// #include <functional>


#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <WebsocketHandler.hpp>

// The HTTPS Server comes in a separate namespace. For easier use, include it here.
using namespace httpsserver;
    #include "WiFi.h"




typedef std::function<void(uint8_t *data, size_t len)> RecvMsgHandler;

// Uncomment to enable webserial debug mode
#define WEBSERVER_DEBUG 1

class WebServices{

public:
    void begin(HTTPServer *server);



private:
    HTTPServer *_server;
    // AsyncWebSocket *_ws;
    RecvMsgHandler _RecvFunc = NULL;
    
    #if defined(WEBSERVER_DEBUG)
        void DEBUG_WEB_SERIAL(const char* message);
    #endif
};

extern WebServices webServices;