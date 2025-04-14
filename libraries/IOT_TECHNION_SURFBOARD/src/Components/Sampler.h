#ifndef SAMPLER_H
#define SAMPLER_H

#include <vector>
#include <string>
#include <Arduino.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
using namespace std;

#include "../Utils/Status.h"
#include "Sensors/SensorBase.h"
#include "IO/SDCardHandler.h"
#include "../Sync/CloudSyncManager.h"


class Sampler {
    // todo: change class to singleton
    private:
        vector<SensorBase*> sensors;
        SamplerStatus status;
        Logger* logger;
        SDCardHandler* sdCardHandler;
        CloudSyncManager* cloudSyncManager;
        bool hasFilesToUpload;

        File currentUploadFile;
        int fileSampleIndex;

    public:
        Sampler(Logger* logger, SDCardHandler* sdCardHandler, CloudSyncManager* cloudSyncManager);

        void addSensor(SensorBase* sensor);

        void init();

        void startSampling(int timestamp);
        void stopSampling();

        bool hasFilesToCloudUpload(){
            return hasFilesToUpload;
        }

        SamplerStatus getStatus();

        // use this when you want the unit to enter error state for external reasons
        void enterErrorState();

        void connect();

        void disconnect();

        bool isConnected();

        // upload the next samples batch
        void uploadNextSamples(); 

        void writeSensorsData();
};

#endif // SAMPLER_H