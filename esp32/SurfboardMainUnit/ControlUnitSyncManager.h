#ifndef CONTROL_UNIT_SYNC_MANAGER_H 
#define CONTROL_UNIT_SYNC_MANAGER_H

using namespace std;
#include <cstdint>
#include <string>
#include <queue>
#include <vector>

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#include <IOT_TECHNION_SURFBOARD.h>

class ControlUnitSyncManager{
    // singleton class
    private:
        static Logger* logger;
        static queue<StatusUpdateMessage> statusUpdateQueue;
        static SemaphoreHandle_t queueMutex;
        static vector<esp_now_peer_info_t*> peers;
        static ControlUnitSyncManager* instance;
        bool isConnected;
        int channel;

        static void processReceivedMessages(const uint8_t *mac_addr, const uint8_t *incomingData, int len);  
        ControlUnitSyncManager(){
            isConnected=false;
            channel=0; // default
        }; 
    public:
        ControlUnitSyncManager(const ControlUnitSyncManager& obj) = delete;
        static ControlUnitSyncManager* getInstance() {
            if (instance == nullptr) {
                instance = new ControlUnitSyncManager();
            }
            return instance;
        }
        void init(uint8_t samplingUnits[][6], int samplingUnitsNum, int channel);
        bool isESPNowConnected(){
            return isConnected;
        }
        void connect();
        void disconnect();
        void sendCommand(const ControlUnitCommand& command,const std::map<string,string>& params, uint8_t samplingUnitMac[6]);
        void broadcastCommand(const ControlUnitCommand& command,const std::map<string,string>& params); 
        bool hasStatusUpdateMessages();
        static void addStatusUpdateMessage(StatusUpdateMessage msg); 
        static StatusUpdateMessage popStatusUpdateMessage(); 
};


#endif /* CONTROL_UNIT_SYNC_MANAGER_H */
