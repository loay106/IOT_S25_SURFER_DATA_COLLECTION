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
const int FILE_UPLOAD_ESP_NOW_CONNECTION_LIMIT_MILLIS = 5000;
const int WIFI_CONNECTION_RETRY_TIMEOUT_MILLIS = 6000;
const int FILE_UPLOAD_STOP_CONNECTION_SWITCH_INTERVAL = 5000;
const int MODE_MINIMUM_TIME_MILLIS = 3000;
const int FILE_UPLOAD_STOP_TIME_MAX_THRESHOLD = 120000;

typedef struct SamplingUnitRep{
    uint8_t mac[6];
    SamplerStatus status;
    bool hasFilesToUpload;
    unsigned long lastCommandSentMillis;
    unsigned long lastStatusUpdateMillis;
    String mDNSHostname;
    String dataCollectorServerIP;
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
        unsigned long lastModeChange;
        unsigned long fileUploadStopStartTime;
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

        void discoverDisconnected();

        void sendESPNowCommand(SamplingUnitRep& unit, ControlUnitCommand command, std::map<String, String> commandParams);        
    public:
        SurfboardMainUnit(ControlUnitSyncManager* syncManager, RTCTimeHandler* timeHandler, RGBStatusHandler* statusLighthandler, ButtonHandler* buttonHandler, Logger* logger, Sampler* sampler, SDCardHandler* sdCardHandler,WirelessHandler* wirelessHandler, const String& _wifiSSID, const String& _wifiPassword, DataCollectorServer* server);
        void init(vector<uint8_t*> samplingUnitsMacAddresses);
        void addSensor(SensorBase* sensor);

        SystemStatus getStatus();

        void handleButtonPress();
        void readStatusUpdateMessages();
        
        void loopSampling();
        void loopFileUpload();
        void loopFileUploadStopping();
        void loopFileUploadWifiError();
        void loopStandby();

        void loopComponents();

};

#endif // SURFBOARD_MAIN_UNIT_H