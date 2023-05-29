#pragma once
#include "WiFi.h"
#ifdef __cplusplus
extern "C"
{
#endif



    // void Wifi_start();
    class RoamingWiFiClass
    {

    public:
        RoamingWiFiClass();
        /**
		 * @brief   This will start a task that will connect WIFI (and reconnect) 
         *          to any of the networks defined in NvsConf or any open network.
         *          The selected network is the one with the strongest signal at
         *          the time of connecting.
		 * @param  
		 */
        void start();
       /**
		 * @brief   Stops connection and connects to the strongest network available. 
         *          This is called automatically when the connection is lost.
		 * @param  
		 */
        void connect();
    private:

    };
    void handleOtherEvents(WiFiEvent_t event, WiFiEventInfo_t info);
        void handleWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    void handleWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
    void handleWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

    extern RoamingWiFiClass RoamingWiFi;

#ifdef __cplusplus
}
#endif
