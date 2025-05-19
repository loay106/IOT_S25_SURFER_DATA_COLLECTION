#ifndef FORCE_HX711_H
#define FORCE_HX711_H

#include "SensorBase.h"
#include "HX711.h"

using namespace std;

const float GRAVITY = 9.81;

class Force_HX711 : public SensorBase { 
    private:
        HX711 sensor;
        int calibrationFactor;
        int doutPin;
        int sckPin;
    public:
        Force_HX711(Logger* logger, SDCardHandler* sdcardHandler, int calibrationFactor, int doutPin, int sckPin): SensorBase(logger, sdcardHandler, "HX711"){
            this->calibrationFactor = calibrationFactor;
            this->doutPin = doutPin;
            this->sckPin = sckPin;
        }
        void enableSensor() override {};

        void disableSensor() override {
            logger->info(F("HX711 sensor disabled"));
        };

        String getSample() override {
            if (sensor.is_ready()) {
                float mass_kg = sensor.get_units() / 1000.0;
                float force_N = mass_kg * GRAVITY;

                // Format to 2 decimal places
                String result = String(force_N, 2);
                return result;
            } else {
                throw NotReadyError();
            }
        }


        void init() override{
            sensor.begin(doutPin, sckPin);
            sensor.set_scale(calibrationFactor);
            sensor.tare(); // Reset scale to zero
        }
};


#endif // FORCE_HX711_H
