#ifndef CONTROL_UNIT_SYNC_MANAGER_H 
#define CONTROL_UNIT_SYNC_MANAGER_H

using namespace std;
#include <cstdint>
#include <queue>
#include <vector>

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>

#include <IOT_TECHNION_SURFBOARD.h>

class ControlUnitSyncManager{
    // singleton class
    private:
        static Logger* logger;
        static queue<StatusUpdateMessage> statusUpdateQueue;
        static SemaphoreHandle_t queueMutex;
        static ControlUnitSyncManager* instance;
        HTTPClient httpClient;
        ControlUnitSyncManager(){}; 
    public:
        ControlUnitSyncManager(const ControlUnitSyncManager& obj) = delete;
        static ControlUnitSyncManager* getInstance() {
            if (instance == nullptr) {
                instance = new ControlUnitSyncManager();
            }
            return instance;
        }
        void sendESPNowCommand(const ControlUnitCommand& command,const std::map<String,String>& params, uint8_t samplingUnitMac[6]);
        void broadcastESPNowCommand(const ControlUnitCommand& command,const std::map<String,String>& params); 
        bool hasStatusUpdateMessages();
        void sendWifiStopFileUploadCommand(const String& unitIP);
        void pingServerWifi(const String& unitIP);
        String resolveHostnameToIP(const String& hostname);
        static void addStatusUpdateMessage(StatusUpdateMessage msg); 
        static StatusUpdateMessage popStatusUpdateMessage(); 
        static void processReceivedESPNowMessages(const uint8_t *mac_addr, const uint8_t *incomingData, int len);

};


#endif /* CONTROL_UNIT_SYNC_MANAGER_H */
