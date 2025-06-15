#ifndef SAMPLING_UNIT_SYNC_MANAGER_H
#define SAMPLING_UNIT_SYNC_MANAGER_H

#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>

#include <IOT_TECHNION_SURFBOARD.h>


using namespace std;

class SamplingUnitSyncManager {
    // todo: change class to singleton
    private:
        static CommandMessage* nextCommand;
        esp_now_peer_info_t* controlUnitPeer;
        uint8_t controlUnitMac[6];
        static Logger* logger;

        static SamplingUnitSyncManager* instance;

        SamplingUnitSyncManager(){};
        
    public:
        SamplingUnitSyncManager(const SamplingUnitSyncManager& obj) = delete;
        static SamplingUnitSyncManager* getInstance() {
            if (instance == nullptr) {
                instance = new SamplingUnitSyncManager();
            }
            return instance;
        }

        void reportStatus(SamplerStatus status);
        static void setNextCommand(CommandMessage cmd);
        static void onDataReceivedCallback(const uint8_t *mac_addr, const uint8_t* incomingData, int len);
        CommandMessage getNextCommand();

};

#endif // SAMPLING_UNIT_SYNC_MANAGER_H
