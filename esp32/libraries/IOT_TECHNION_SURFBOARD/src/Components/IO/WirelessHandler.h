#ifndef WIRELESS_HANDLER_H 
#define WIRELESS_HANDLER_H

#include <vector>

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "../../Utils/Adresses.h"
#include "../../Utils/Exceptions.h"

#include "Logger.h"

typedef void (*ESPNowRecvCallback)(const uint8_t *mac, const uint8_t *data, int len);

class WirelessHandler{
    public:
        enum MODE {
            // desired status
            WIFI,
            ESP_NOW,
            OFF
        };

    WirelessHandler(Logger* logger, const String& _wifiSSID, const String& _wifiPassword, int _esp_now_channel, std::vector<uint8_t*> _esp_now_peers, ESPNowRecvCallback _esp_rec_ballback);
    MODE getCurrentMode();
    void switchToWifi();
    void switchToESPNow();
    bool isConnected();
    String getIP();
    String getMacAddress();
      
    void loop();

    private:
        enum STATUS{
            // current status
            DISCONNECTED, 

            WIFI_DISCONNECTING, // 100 ms delay
            WIFI_CONNECTING, // 300 ms delay between retries
            WIFI_CONNECTED,

            ESP_NOW_DISCONNECTING, // 100 ms delay
            ESP_NOW_CONNECTING, // 100 ms delay
            ESP_NOW_CONNECTED
        };

        String wifi_ssid;
        String wifi_password;

        int esp_now_channel;
        std::vector<esp_now_peer_info_t *> esp_now_peers;
        ESPNowRecvCallback esp_rec_ballback;

        MODE currentMode;
        STATUS currentStatus;

        Logger* logger;
        int timeToDelayMillis;
        int delayStartTimeMillis;

        void disconnectWifi();
        void connectWifi();

        void disconnectEspNow();
        void connectEspNow();


        bool isInDelay();
        void setDelayPeriod(int delayTimeMillis);
};


#endif /* WIRELESS_HANDLER_H */