#include "RoamingWifi.h"

#include "Arduino.h"
#include "WiFi.h"
#include "confnvs.h"

static int retries = 0;

RoamingWiFiClass::RoamingWiFiClass(){};

void RoamingWiFiClass::start()
{
    wifi_mode_t wifiMode = WIFI_AP;
    if (NVS.getInt(CONFIG_ITEMS[CONFIG_WIFI_STA_ACTIVE].key))
    {
        if (NVS.getInt(CONFIG_ITEMS[CONFIG_WIFI_AP_ACTIVE].key))
        {
            wifiMode = WIFI_AP_STA;
        }
        else
        {
            wifiMode = WIFI_STA;
        }
    }

    WiFi.mode(wifiMode);
    String apName = NVS.getString(CONFIG_ITEMS[CONFIG_WIFI_AP_SSID].key);
    if (apName == "")
    {
        apName = "EspMower";
    }
    WiFi.softAP(apName.c_str(), NULL);
    WiFi.disconnect();
    delay(100);
    //  WiFi.mode(WIFI_STA);
    WiFi.onEvent(handleWiFiStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(handleWiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(handleWiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    // WiFi.onEvent(handleOtherEvents, ARDUINO_EVENT_MAX);

    connect();
};

void RoamingWiFiClass::connect()
{
    retries++;
    if (retries > 3)
    {
        WiFi.mode(WIFI_AP);
        return;
    }
    else
    {

        WiFi.scanDelete();
        Serial.println("scan start");

        // WiFi.scanNetworks will return the number of networks found
        int n = WiFi.scanNetworks();
        Serial.println("scan done");
        if (n == 0)
        {
            Serial.println("no networks found");
        }
        else
        {
            Serial.print(n);
            Serial.println(" networks found");
            for (int i = 0; i < n; ++i)
            {
                // Print SSID and RSSI for each network found
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(WiFi.SSID(i));
                Serial.print(" (");
                Serial.print(WiFi.RSSI(i));
                Serial.print(")");
                Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
                delay(10);
            }
            if (0)
            { // static ip address
                // Set your Static IP address
                IPAddress local_IP(192, 168, 1, 184);
                // Set your Gateway IP address
                IPAddress gateway(192, 168, 1, 1);

                IPAddress subnet(255, 255, 0, 0);
                IPAddress primaryDNS(8, 8, 8, 8);   // optional
                IPAddress secondaryDNS(8, 8, 4, 4); // optional
                // Configures static IP address
                if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
                {
                    Serial.println("STA Failed to configure");
                }
            }

            for (int ii = 0; ii < n; ii++)
            {
                for (int jj = 0; jj < 3; jj++)
                {
                    String ssid = NVS.getString(CONFIG_ITEMS[CONFIG_WIFI_STA_SSID + (2 * jj)].key);
                    String passwd = NVS.getString(CONFIG_ITEMS[CONFIG_WIFI_STA_PASSWORD + (2 * jj)].key);
                    Serial.println("Checking network : " + ssid + "  " + "*******");
                    if (WiFi.SSID(ii) == ssid)
                    {
                        WiFi.begin(ssid.c_str(), passwd.c_str());
                        Serial.print("Connecting to WiFi ..");
                        int timeout = 0;
                        while (WiFi.status() != WL_CONNECTED && timeout < 10)
                        {
                            Serial.print('.');
                            delay(1000);
                            timeout++;
                        }
                        //Serial.println(WiFi.localIP());

                        Serial.println("Wifi setup done");
                        return;
                    }
                }
                ii++;
            }

            Serial.println("Couldn't connect to any known Wifi.  Use ap instead.");
        }
    }
}

RoamingWiFiClass RoamingWiFi;

void handleOtherEvents(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("other event triggered");
    Serial.println(event);
}
void handleWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Connected to AP successfully!");
}

void handleWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void handleWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
    RoamingWiFi.connect();
}
