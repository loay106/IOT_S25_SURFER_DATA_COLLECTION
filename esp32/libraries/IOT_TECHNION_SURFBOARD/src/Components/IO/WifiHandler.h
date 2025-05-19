#ifndef WIFI_HANDLER_H 
#define WIFI_HANDLER_H

#include "Logger.h"
#include <WiFi.h>
#include "../../Utils/Adresses.h"

class WifiHandler {
  private:
    unsigned long lastAttemptTime = 0;
    const unsigned long retryInterval = 5000; // retry every 5 seconds
    bool isConnecting = false;
    String ssid, password;
    Logger* logger;

  public:
    WifiHandler(Logger* logger, const char* _ssid, const char* _password) {
      this->logger = logger;
      ssid = _ssid;
      password = _password;
    }

    void connect() {
      logger->info("Starting WiFi connection...");
      WiFi.mode(WIFI_STA);
      WiFi.setAutoReconnect(true);
      WiFi.persistent(true);
      WiFi.begin(ssid.c_str(), password.c_str());
      isConnecting = true;
      lastAttemptTime = millis(); // set time for next retry
    }

    void reconnect(){
      if (!isConnecting || WiFi.status() == WL_CONNECTED) return;

      if (millis() - lastAttemptTime > retryInterval) {
        lastAttemptTime = millis();

        logger->info("Retrying WiFi connection...");
        WiFi.disconnect(); // force disconnect before retry
        WiFi.begin(ssid.c_str(), password.c_str());
        isConnecting=true;
      }

      if(WiFi.status() == WL_CONNECTED){
        isConnecting=false;
      }
    }

    String getIP(){
      return WiFi.localIP().toString();
    }

    bool isConnected() const {
      return WiFi.status() == WL_CONNECTED;
    }

    void disconnect() {
      logger->info("Disconnecting from WiFi...");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      logger->info("WiFi disconnected and turned off.");
    }

    String getMacAddress(){
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        return macToString(mac);
    }
};

#endif /* WIFI_HANDLER_H */
