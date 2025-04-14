#ifndef WIFI_HANDLER_H 
#define WIFI_HANDLER_H

#include "Logger.h"
#include <WiFi.h>

class WifiHandler {
private:
  unsigned long lastAttemptTime = 0;
  const unsigned long retryInterval = 500;
  bool isConnecting = false;
  String ssid, password;

public:
  void startConnection(const char* _ssid, const char* _password) {
    ssid = _ssid;
    password = _password;
    Logger::getInstance()->info("Starting WiFi connection...");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid.c_str(), password.c_str());
    isConnecting = true;
  }

  void update() {
    if (!isConnecting || WiFi.status() == WL_CONNECTED) return;

    if (millis() - lastAttemptTime > retryInterval) {
      lastAttemptTime = millis();
    }

    if (WiFi.status() == WL_CONNECTED) {
      isConnecting = false;
      Logger::getInstance()->info("WiFi connected! IP: ");
      Logger::getInstance()->info(WiFi.localIP().toString().c_str());
    }
  }

  bool isConnected() const {
    return WiFi.status() == WL_CONNECTED;
  }

  void disconnect() {
    Logger::getInstance()->info("Disconnecting from WiFi...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Logger::getInstance()->info("WiFi disconnected and turned off.");
  }
};


#endif /* WIFI_HANDLER_H */
