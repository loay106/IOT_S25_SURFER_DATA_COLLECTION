#ifndef SENSOR_BASE_H
#define SENSOR_BASE_H

#include <Arduino.h>
#include <string>
#include <sstream>

#include "../IO/SDCardHandler.h"
#include "../IO/Logger.h"
#include "../../Utils/Exceptions.h"

using namespace std;

enum class SensorStatus{
    STANDBY,
    SAMPLING,
    ERROR,
};

const int MAX_SAMPLES_BUFFER_LENGTH = 4096;

class SensorBase{
    private:
        SDCardHandler* sdcardHandler;
        string* sampleBuffer;

    protected:
        Logger* logger;
        string model;
        string* samplingFileName;
        unsigned long samplesCount;
        unsigned long samplingStartMillis;
        void flushSamplesBuffer(bool isLastLine);

    public:
        SensorBase(){};
        SensorBase(Logger* logger, SDCardHandler* sdcardHandler, string model);

        string getModel();
        void startSampling(string outputFilePath);
        void stopSampling();
        void writeSamples();

        virtual void enableSensor() = 0;
        virtual void disableSensor() = 0;
        virtual string getSample() = 0;
        virtual void init() = 0;
};

#endif // SENSOR_BASE_H