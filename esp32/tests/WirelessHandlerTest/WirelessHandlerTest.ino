#include <IOT_TECHNION_SURFBOARD.h>

WirelessHandler* wirelessHandler = nullptr;
Logger* logger = nullptr;

String WIFI_SSID = "TestPhone";
String WIFI_PASSWORD = "test123456";
int ESP_NOW_CHANNEL = 0;
std::vector<uint8_t*> esp_now_peers;

unsigned long lastSwitch;

void onESPNowData(const uint8_t *mac, const uint8_t *data, int len) {
    Serial.print("Received from: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();

    Serial.print("Data: ");
    for (int i = 0; i < len; i++) {
        Serial.print((char)data[i]);
    }
    Serial.println();
}

void setup() {
    logger = Logger::getInstance();
    logger->init(115200);
    logger->setLogLevel(LogLevel::DEBUG);
    wirelessHandler = new WirelessHandler(logger, WIFI_SSID, WIFI_PASSWORD, ESP_NOW_CHANNEL, esp_now_peers, onESPNowData);
    lastSwitch = 0;
}

void loop() {
    wirelessHandler->loop();
    // switch modes every 30 seconds
    if (millis() - lastSwitch > 30000) {
      WirelessHandler::MODE mode = wirelessHandler->getCurrentMode();
      logger->info("Current mode: " + mode_to_string(mode));
      if(mode == WirelessHandler::MODE::WIFI){
          wirelessHandler->switchToESPNow();
      }else{
          wirelessHandler->switchToWifi();
      }
      lastSwitch = millis();
    }
}
