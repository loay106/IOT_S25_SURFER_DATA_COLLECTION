#ifndef SURFBOARD_MAIN_UNIT_H
#define SURFBOARD_MAIN_UNIT_H

#include <vector>
#include <map>
using namespace std;

#include <Arduino.h>
#include <IOT_TECHNION_SURFBOARD.h>
#include "ControlUnitSyncManager.h"

const int COMMAND_SEND_MIN_INTERVAL_MILLIS = 500; 
const int MAX_STATUS_UPDATE_DELAY = 3000;

typedef struct SamplingUnitRep{
    uint8_t mac[6];
    SamplerStatus status;
    bool hasFilesToUpload;
    unsigned long lastCommandSentMillis;
    unsigned long lastStatusUpdateMillis;
    String mDNSHostname;
} SamplingUnitRep;

class SurfboardMainUnit {
    private:
        std::map<String, SamplingUnitRep> samplingUnits; // mac string to instance mapping
        ControlUnitSyncManager* syncManager;
        RTCTimeHandler* timeHandler;
        RGBStatusHandler* statusLighthandler;
        ButtonHandler* buttonHandler;
        Logger* logger;
        Sampler* sampler; // internal sampler
        SDCardHandler* sdCardHandler;
        unsigned long currentSamplingSession;
        SystemStatus status;
        WirelessHandler* wirelessHandler;
        String WIFI_SSID;
        String WIFI_PASSWORD;
        DataCollectorServer* server;

        void updateStatus(SystemStatus newStatus);

        void startSampling();
        void stopSampling();

        void startSampleFilesUpload();
        void stopSampleFilesUpload();

        void sendCommand(SamplingUnitRep& unit, ControlUnitCommand command, std::map<String, String> commandParams);        
    public:
        SurfboardMainUnit(ControlUnitSyncManager* syncManager, RTCTimeHandler* timeHandler, RGBStatusHandler* statusLighthandler, ButtonHandler* buttonHandler, Logger* logger, Sampler* sampler, SDCardHandler* sdCardHandler,WirelessHandler* wirelessHandler, DataCollectorServer* server);
        void init(vector<uint8_t*> samplingUnitsMacAddresses);
        void addSensor(SensorBase* sensor);

        SystemStatus getStatus();

        void handleButtonPress();
        void readStatusUpdateMessages();
        
        void loopSampling();
        void loopFileUpload();
        void loopStandby();
        void loopDiscoverDisconnected();

        void loopComponents();

};

#endif // SURFBOARD_MAIN_UNIT_H