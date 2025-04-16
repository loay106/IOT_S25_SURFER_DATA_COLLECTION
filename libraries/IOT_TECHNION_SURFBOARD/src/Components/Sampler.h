#ifndef SAMPLER_H
#define SAMPLER_H

#include <vector>
#include <string>
#include <Arduino.h>

#include "Sensors/SensorBase.h"
#include "IO/SDCardHandler.h"

using namespace std;

class Sampler {
    private:
        vector<SensorBase*> sensors;
        Logger* logger;
        SDCardHandler* sdCardHandler;
        bool _isSampling;

    public:
        Sampler(Logger* logger, SDCardHandler* sdCardHandler);

        void addSensor(SensorBase* sensor);
        void init();
        void startSampling(unsigned long timestamp);
        void stopSampling();
        bool isSampling();
        void writeSensorsData();
};

#endif // SAMPLER_H