#ifndef SURFBOARD_MAIN_UNIT_H
#define SURFBOARD_MAIN_UNIT_H

#include <vector>
#include <map>
#include <string>
using namespace std;

#include <Arduino.h>

#include <IOT_TECHNION_SURFBOARD.h>

/*
  todo:
    1. handle edge cases with wifi/file upload
    2. fix button press
    3. finish sampling unit
    4. test with 1 sampling unit or more
    5. connect to wifi, get wifi channel and set it for esp now 
*/

const int COMMAND_SEND_MIN_INTERVAL_MILLIS = 500; 
const int MAX_STATUS_UPDATE_DELAY = 3000;

typedef struct SamplingUnitRep{
    uint8_t mac[6];
    SamplerStatus status;
    bool hasFilesToUpload;
    unsigned long lastCommandSentMillis;
    unsigned long lastStatusUpdateMillis;
} SamplingUnitRep;

class SurfboardMainUnit {
    private:
        std::map<string, SamplingUnitRep> samplingUnits; // mac string to instance mapping
        ControlUnitSyncManager* syncManager;
        RTCTimeHandler* timeHandler;
        RGBStatusHandler* statusLighthandler;
        ButtonHandler* buttonHandler;
        Logger* logger;
        Sampler* sampler; // internal sampler
        SDCardHandler* sdCardHandler;
        int currentSamplingSession;
        unsigned long uploadStartTime;

        SystemStatus status;

        void updateStatus(SystemStatus newStatus);

        void startSampling();
        void stopSampling();

        void startSampleFilesUpload();
        void stopSampleFilesUpload();

        void sendCommand(SamplingUnitRep& unit, ControlUnitCommand command);
    public:
        SurfboardMainUnit(ControlUnitSyncManager* syncManager, RTCTimeHandler* timeHandler, RGBStatusHandler* statusLighthandler, ButtonHandler* buttonHandler, Logger* logger, Sampler* sampler, SDCardHandler* sdCardHandler);
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