#include "WirelessHandler.h"

WirelessHandler::WirelessHandler(Logger* logger, int _esp_now_channel, std::vector<uint8_t*> _esp_now_peers, ESPNowRecvCallback _esp_rec_ballback){
    this->logger = logger;
    this->wifi_ssid = "";
    this->wifi_password = "";
    this->esp_now_channel = _esp_now_channel;
    for (int i = 0; i < _esp_now_peers.size(); i++) {
        esp_now_peer_info_t* peerInfo = new esp_now_peer_info_t();
        memcpy(peerInfo->peer_addr, _esp_now_peers[i], 6);
        peerInfo->channel = _esp_now_channel;
        peerInfo->encrypt = false;
        this->esp_now_peers.push_back(peerInfo);
    }
    this->esp_rec_ballback = _esp_rec_ballback;
    this->currentMode = WirelessHandler::MODE::OFF;
    this->currentStatus = WirelessHandler::STATUS::DISCONNECTED;
    this->timeToDelayMillis = 0;
    this->delayStartTimeMillis = 0;
    this->currentModeStartTime - 0;
}

WirelessHandler::MODE WirelessHandler::getCurrentMode(){
    WirelessHandler::MODE mode = currentMode;
    return mode;
}

void WirelessHandler::switchToWifi(const String& _wifiSSID, const String& _wifiPassword){
    if(this->currentMode != WirelessHandler::MODE::WIFI){
        this->wifi_ssid = _wifiSSID;
        this->wifi_password = _wifiPassword;
        logger->info("Switching to WiFi connection...");
        currentModeStartTime = millis();
        this->currentMode = WirelessHandler::MODE::WIFI;
    }
}

void WirelessHandler::switchToESPNow(){
    if(this->currentMode != WirelessHandler::MODE::ESP_NOW){
    logger->info("Switching to ESP NOW connection...");
        currentModeStartTime = millis();
        this->currentMode = WirelessHandler::MODE::ESP_NOW;
    }
}

bool WirelessHandler::isConnected(){
    return ((currentMode == WirelessHandler::MODE::ESP_NOW && currentStatus == WirelessHandler::STATUS::ESP_NOW_CONNECTED) 
        || (currentMode == WirelessHandler::MODE::WIFI && currentStatus == WirelessHandler::STATUS::WIFI_CONNECTED));
}

String WirelessHandler::getIP(){
    return WiFi.localIP().toString();
}

String WirelessHandler::getMacAddress(){
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    return macToString(mac);
}

unsigned long WirelessHandler::getCurrentModeStartTime(){
    return currentModeStartTime;
}

void WirelessHandler::loop(){
    if(isInDelay()){
        return;
    }

    if(currentMode == WirelessHandler::MODE::WIFI){
        switch(currentStatus){
            case WirelessHandler::STATUS::DISCONNECTED:
                disconnectWifi();
                return;
            case WirelessHandler::STATUS::WIFI_DISCONNECTING:
                connectWifi();
                return;
            case WirelessHandler::STATUS::WIFI_CONNECTING:
                if(WiFi.status() == WL_CONNECTED){
                    currentStatus = WirelessHandler::STATUS::WIFI_CONNECTED;
                    logger->info(String("Wifi connected! IP:" + getIP()));
                }else{
                    disconnectWifi();
                }
                return;
            case WirelessHandler::STATUS::WIFI_CONNECTED:
                if(WiFi.status() != WL_CONNECTED){
                    connectWifi();
                }
                return;
            case WirelessHandler::STATUS::ESP_NOW_DISCONNECTING:
                disconnectWifi();
                return;
            case WirelessHandler::STATUS::ESP_NOW_CONNECTING:
            case WirelessHandler::STATUS::ESP_NOW_CONNECTED:
                disconnectEspNow();
                return;

        }
    }else if(currentMode == WirelessHandler::MODE::ESP_NOW){
        // handle esp_now connection
        switch(currentStatus){
            case WirelessHandler::STATUS::DISCONNECTED:
                disconnectWifi();
                return;
            case WirelessHandler::STATUS::WIFI_DISCONNECTING:
                connectEspNow();
                return;
            case WirelessHandler::STATUS::WIFI_CONNECTING:
                disconnectWifi();
                return;
            case WirelessHandler::STATUS::WIFI_CONNECTED:
                disconnectWifi();
                return;
            case WirelessHandler::STATUS::ESP_NOW_DISCONNECTING:
                connectEspNow();
                return;
            case WirelessHandler::STATUS::ESP_NOW_CONNECTING:
                currentStatus = WirelessHandler::STATUS::ESP_NOW_CONNECTED;
                logger->info(String("ESP Now connected!"));
                return;
            case WirelessHandler::STATUS::ESP_NOW_CONNECTED:
                return;
        }
    }
}

void WirelessHandler::disconnectWifi(){
    logger->info(F("Disconnecting from WiFi..."));
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    currentStatus = WirelessHandler::STATUS::WIFI_DISCONNECTING;
    setDelayPeriod(100);
}

void WirelessHandler::connectWifi(){
    if(WiFi.status() == WL_CONNECTED){
        return;
    }else{
        logger->info("Starting WiFi connection...Connecting to " + wifi_ssid);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
        currentStatus = WirelessHandler::STATUS::WIFI_CONNECTING;
        setDelayPeriod(500);
    }
}

void WirelessHandler::disconnectEspNow(){
    logger->info("Disconnecting esp now connection...");
    WiFi.disconnect(true);
    esp_now_deinit();
    currentStatus = WirelessHandler::STATUS::ESP_NOW_DISCONNECTING;
    setDelayPeriod(100);
}

void WirelessHandler::connectEspNow(){
    //WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    if(esp_now_init() != ESP_OK){
        throw ESPNowSyncError();
    }
    for(esp_now_peer_info_t* peerInfo: esp_now_peers){
        if (esp_now_add_peer(peerInfo) != ESP_OK) {
            throw ESPNowSyncError();
        }
    }
    esp_now_register_recv_cb(esp_rec_ballback);
    currentStatus = WirelessHandler::STATUS::ESP_NOW_CONNECTING;
    setDelayPeriod(100);
}

bool WirelessHandler::isInDelay(){
    return (millis() - delayStartTimeMillis) <= timeToDelayMillis;
}

void WirelessHandler::setDelayPeriod(int delayTimeMillis){
    this->timeToDelayMillis = delayTimeMillis;
    this->delayStartTimeMillis = millis();
}
