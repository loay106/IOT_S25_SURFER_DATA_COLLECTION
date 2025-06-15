#ifndef SURFBOARD_SAMPLING_UNIT_H
#define SURFBOARD_SAMPLING_UNIT_H

#include <vector>

#include <IOT_TECHNION_SURFBOARD.h>
#include "SamplingUnitSyncManager.h"

extern const unsigned long MAX_WIFI_CONNECT_TRY_MILLIS;

class SurfboardSamplingUnit {
    private:
        Sampler* sampler;
        SamplingUnitSyncManager* syncManager;
        Logger* logger;
        SDCardHandler* sdCardHandler;
        WirelessHandler* wirelessHandler;
        DataCollectorServer* server;
        SamplerStatus status;
        unsigned long currentSamplingSession;
        String WIFI_SSID;
        String WIFI_PASSWORD;
        int lastStatusReportTime;
        unsigned long wifi_connection_start_time;
    public:
        SurfboardSamplingUnit(WirelessHandler* wirelessHandler, SamplingUnitSyncManager* syncManager, SDCardHandler* sdCardHandler, Sampler* sampler, Logger* logger, DataCollectorServer* _server);
        void addSensor(SensorBase* sensor);
        void handleNextCommand();
        SamplerStatus getStatus();

        void loopSampling();
        void loopFileUpload();
        void loopStandBy();
        void reportStatus(bool force=false);
        void loopComponents();
};
#endif // SURFBOARD_SAMPLING_UNIT_H
