#ifndef FORCE_FAKE_H
#define FORCE_FAKE_H

#include "SensorBase.h"

using namespace std;

class Mock_HX711 : public SensorBase { 
    private:
        bool sensor_enabled;
        unsigned long lastRetrieved;
        int delayBetweenSamples;
    public:
        Mock_HX711(Logger* logger, SDCardHandler* sdcardHandler, int rate): SensorBase(logger, sdcardHandler, "Mock-HX711"),
        sensor_enabled(false), lastRetrieved(0){
            delayBetweenSamples = 1000/rate;
        }
        void enableSensor() override {
            sensor_enabled=true;
        };

        void disableSensor() override {
            sensor_enabled=false;
            logger->debug(F("Mock HX711 sensor disabled"));
        };

        String getSample() override {
            unsigned long current = millis();
            if (sensor_enabled && (current - lastRetrieved) >= delayBetweenSamples) {
                float force = random(0, 1000) * 9.81;
                lastRetrieved = current;
                return String(force, 2);  // 2 decimal places
            } else {
                throw NotReadyError();
            }
        }


        void init() override{}
};


#endif // FORCE_FAKE
