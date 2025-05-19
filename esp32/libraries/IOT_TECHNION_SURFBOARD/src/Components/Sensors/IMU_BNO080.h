#ifndef IMU_BNO080_H
#define IMU_BNO080_H

#include "SensorBase.h"
#include "SparkFun_BNO080_Arduino_Library.h"

class IMU_BNO080: public SensorBase {
    private:
        BNO080 sensor;
        int sampleDelay;
        static void IRAM_ATTR IMU_BNO080_ISR(void* arg) {
            bool* flag = static_cast<bool*>(arg);
            *flag=true;
        }
    public:
        IMU_BNO080(Logger* logger, SDCardHandler* sdcardHandler, int rate)
            : SensorBase(logger, sdcardHandler, "BNO080") {
            sampleDelay = 1000/rate;
        }
        void enableSensor() override {
            sensor.enableAccelerometer(sampleDelay);
        }
        void disableSensor() override {
            sensor.enableAccelerometer(0);
            logger->info(F("BNO080 sensor disabled"));
        }
        void init() override {
            Wire.begin(21, 22);
            if (sensor.begin() == false){
                logger->error(F("BNO080 not detected at default I2C address. Check your jumpers and the hookup guide"));
                throw InitError();
            }
            Wire.setClock(400000);
        }

        string getSample() override {
            if(sensor.dataAvailable()) {
                float accX = sensor.getAccelX();
                float accY = sensor.getAccelY();
                float accZ = sensor.getAccelZ();
                ostringstream oss;
                oss.precision(2);
                oss << std::fixed << accX << " " << accY << " " << accZ;
                return oss.str();
            } else {
                throw NotReadyError();
            }
        }
};

#endif // IMU_BNO080_H
