#ifndef SAMPLING_UNIT_SYNC_MANAGER_H
#define SAMPLING_UNIT_SYNC_MANAGER_H

#include <WiFi.h>
#include <esp_now.h>
#include <string>
#include <Arduino.h>

#include "../Components/IO/Logger.h"
#include "SyncMessages.h"


using namespace std;

class SamplingUnitSyncManager {
    // todo: change class to singleton
    private:
        static CommandMessage* nextCommand;
        esp_now_peer_info_t* controlUnitPeer;
        uint8_t controlUnitMac[6];
        static Logger* logger;
        bool isConnected;
        int channel;

        static SamplingUnitSyncManager* instance;

        SamplingUnitSyncManager(){
            isConnected=false;
            channel=0; // default channel
        };
        static void onDataReceivedCallback(const uint8_t *mac_addr, const uint8_t* incomingData, int len);
    public:
        SamplingUnitSyncManager(const SamplingUnitSyncManager& obj) = delete;
        static SamplingUnitSyncManager* getInstance() {
            if (instance == nullptr) {
                instance = new SamplingUnitSyncManager();
            }
            return instance;
        }

        void init(uint8_t controlUnitMac[], int channel);
        void connect();
        void disconnect();
        void reportStatus(SamplingUnitStatusMessage status);
        static void setNextCommand(CommandMessage cmd);

        CommandMessage getNextCommand();

};

#endif // SAMPLING_UNIT_SYNC_MANAGER_H
