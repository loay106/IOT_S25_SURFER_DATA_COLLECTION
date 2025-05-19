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
        WifiHandler* wifiHandler;
        String WIFI_SSID;
        String WIFI_PASSWORD;
        DataCollectorServer* server;

        void updateStatus(SystemStatus newStatus);

        void startSampling();
        void stopSampling();

        void startSampleFilesUpload();
        void stopSampleFilesUpload();

        void sendCommand(SamplingUnitRep& unit, ControlUnitCommand command);

        bool sendStopUploadToSamplingUnitDataServer(String hostname);
        bool pingSamplingUnitDataServer(String hostname);
        
    public:
        SurfboardMainUnit(ControlUnitSyncManager* syncManager, RTCTimeHandler* timeHandler, RGBStatusHandler* statusLighthandler, ButtonHandler* buttonHandler, Logger* logger, Sampler* sampler, SDCardHandler* sdCardHandler,WifiHandler* wifiHandler, String _wifi_ssid, String _wifi_password, DataCollectorServer* server);
        void init(uint8_t samplingUnitsAdresses[][6], int samplingUnitsNum);
        void addSensor(SensorBase* sensor);

        SystemStatus getStatus();

        void handleButtonPress();
        void readStatusUpdateMessages();
        
        void loopSampling();
        void loopFileUpload();
        void loopStandby();
        void loopDiscoverDisconnected();


};

#endif // SURFBOARD_MAIN_UNIT_H