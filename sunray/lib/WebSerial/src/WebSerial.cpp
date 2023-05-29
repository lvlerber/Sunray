#include "WebSerial.h"
#define DEBUG
// #include <WebsocketHandler.hpp>
#define MAX_CLIENTS 3

void WebConsoleInputHandler(uint8_t *data, size_t len);
#if defined(WEBSERIAL_DEBUG)
void DEBUG_WEB_SERIAL(const char *message)
{
    Serial.print("[WebSerial] ");
    Serial.println(message);
}
#endif

class WebSerialHandler : public WebsocketHandler
{
public:
    // This method is called by the webserver to instantiate a new handler for each
    // client that connects to the websocket endpoint
    static WebsocketHandler *create();
    // This method is called when a message arrives
    void onMessage(WebsocketInputStreambuf *input);
    // Handler function on connection close
    void onClose();
};

// Simple array to store the active clients:
WebSerialHandler *activeClients[MAX_CLIENTS];

// In the create function of the handler, we create a new Handler and keep track
// of it using the activeClients array
WebsocketHandler * WebSerialHandler::create()
    {
#if defined(WEBSERIAL_DEBUG)
        DEBUG_WEB_SERIAL("Client connection received");
#endif
        WebSerialHandler *handler = new WebSerialHandler();
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (activeClients[i] == nullptr)
            {
                activeClients[i] = handler;
                break;
            }
        }
        return handler;
    }

    void WebSerialHandler::onMessage(WebsocketInputStreambuf *input)
    {
#if defined(WEBSERIAL_DEBUG)
        DEBUG_WEB_SERIAL("Received Websocket Data");
#endif
        std::ostringstream ss;
        String cmd;
        ss << input;
        cmd = ss.str().c_str();
        WebConsoleInputHandler((uint8_t *) cmd.c_str( ),cmd.length());
    };


   void WebSerialHandler::onClose()
    {
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
#if defined(WEBSERIAL_DEBUG)
            DEBUG_WEB_SERIAL("Client disconnected");
#endif
            if (activeClients[i] == this)
            {
                activeClients[i] = nullptr;
            };
        }
    }

void WebSerialClass::begin(HTTPServer *server, const char *url)
{

    _server = server;
    WebsocketNode *_ws = new WebsocketNode("/webserialws", &WebSerialHandler::create);
    _server->registerNode(_ws);

/**
 * this document is now served from the sd card 
    _server->registerNode(new ResourceNode("/webserial", "GET", [](HTTPRequest *req, HTTPResponse *res)
                                           {
        // Send Webpage
            const String html="<!DOCTYPE html><html><head><script type = 'text/javascript'>const ws = new WebSocket('ws://'+location.hostname+'/webserialws');\
ws.onopen = function() {document.getElementById('div1').insertAdjacentHTML('beforeend','Connection opened<br>');};ws.onclose = function()  {document.getElementById('div1').insertAdjacentHTML('beforeend','Connection closed<br>');};\
ws.onmessage = function(event) {\
	for(var i=0;i<event.data.length;i++){var c=event.data.charAt(i);\
    if (c!='\\n') {const p = document.createTextNode(c); document.getElementById('div1').appendChild(p);}\
    else {const b = document.createElement('br');document.getElementById('div1').appendChild(b);}}\
document.getElementById('buttonsend').onclick = function(){	ws.send(document.getElementById('cmd').value); \
	document.getElementById('cmd').value='';};};</script><div> <input id='cmd' type='text' placeholder='type message and press the Send Button' style='width:90%'/>\
<input id='buttonsend' type='button' value='Send' /></div></head><body><div id='div1' /></body></html>";
//        Serial.printf("%s\n",html.c_str());

        res->setHeader("Content-Type", "text/html");
        res->print(html); }));
 */


#if defined(WEBSERIAL_DEBUG)
    DEBUG_WEB_SERIAL("Attached AsyncWebServer along with Websockets");
#endif
}


void WebSerialClass::textAll( uint8_t *m, size_t len){
 // Send it back to every client
  for(int i = 0; i < MAX_CLIENTS; i++) {
    if (activeClients[i] != nullptr) {
      activeClients[i]->send(m,len, 2U );  //SEND_TYPE_TEXT
    }
  }
    };  

void WebSerialClass::textAll(const String  m) {
    textAll((uint8_t *) m.c_str(), m.length());
}

void WebSerialClass::textAll( char*  m) {
    textAll((uint8_t *) m, strlen(m));
}


void WebSerialClass::msgCallback(RecvMsgHandler _recv)
{
    _RecvFunc = _recv;
}

// Print
void WebSerialClass::print(String m)
{
    textAll(m);
}

void WebSerialClass::print(const char *m)
{
    textAll(m);
}

void WebSerialClass::print(char *m)
{
    textAll(m);
}

void WebSerialClass::print(int m)
{
    textAll(String(m));
}

void WebSerialClass::print(uint8_t m)
{
    textAll(String(m));
}

void WebSerialClass::print(uint16_t m)
{
    textAll(String(m));
}

void WebSerialClass::print(uint32_t m)
{
    textAll(String(m));
}

void WebSerialClass::print(double m)
{
    textAll(String(m));
}

void WebSerialClass::print(float m)
{
    textAll(String(m));
}

void WebSerialClass::print(const uint8_t *buffer, size_t size)
{
    textAll((uint8_t *) buffer, size);
}

// Print with New Line

void WebSerialClass::println(String m)
{
    textAll(m + "\n");
}

void WebSerialClass::println(const char *m)
{
    textAll(String(m) + "\n");
}

void WebSerialClass::println(char *m)
{
    textAll(String(m) + "\n");
}

void WebSerialClass::println(int m)
{
    textAll(String(m) + "\n");
}

void WebSerialClass::println(uint8_t m)
{
    textAll(String(m) + "\n");
}

void WebSerialClass::println(uint16_t m)
{
    textAll(String(m) + "\n");
}

void WebSerialClass::println(uint32_t m)
{
    textAll(String(m) + "\n");
}

void WebSerialClass::println(float m)
{
    textAll(String(m) + "\n");
}

void WebSerialClass::println(double m)
{
    textAll(String(m) + "\n");
}



int multi_writefn(void *cookie, const char *data, int n)
{
    // Dit is de functie die opgeroepen wordt voor writes naar stdout en stderr
    //
    if (WebSerialActive)
    {
        /* redirect the bytes somewhere; writing to Serial just for an example */
        //   Serial.print("got ");
        //   Serial.print(n);
        //   Serial.print(" bytes:");
        //   Serial.print("    > ");
        char cmdstr[n];
        memcpy(cmdstr, data, n);
        cmdstr[n] = 0;

        Serial.print(cmdstr);
        //   WebSerial.println(cmdstr);
    }
    return n;
}

WebSerialClass WebSerial;