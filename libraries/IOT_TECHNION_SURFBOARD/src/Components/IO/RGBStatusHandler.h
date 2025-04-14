#ifndef RGB_STATUS_HANDLER_H 
#define RGB_STATUS_HANDLER_H

#include "Logger.h"
#include <Arduino.h>

extern const int FLICKERING_RATE;

enum RGBColors{
    NO_COLOR,
    RED,
    GREEN,
    BLUE, 
    CYAN,
    WHITE
};

class RGBStatusHandler{
    private:
        static RGBStatusHandler* instance;
        RGBStatusHandler(){};
    public:
        RGBStatusHandler(const RGBStatusHandler& obj) = delete;
        static RGBStatusHandler* getInstance() {
            if (instance == nullptr) {
                instance = new RGBStatusHandler();
            }
            return instance;
        }
        void init(int redPin, int greenPin, int bluePin);
        void updateColors(RGBColors first, RGBColors second);
};


#endif /* RGB_STATUS_HANDLER_H */