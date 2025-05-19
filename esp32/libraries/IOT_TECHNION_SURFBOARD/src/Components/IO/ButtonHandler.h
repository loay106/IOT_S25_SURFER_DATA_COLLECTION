#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "Logger.h"
#include <Arduino.h>

enum ButtonPressType{
    NO_PRESS,
    LONG_PRESS,
    SOFT_PRESS
};

class ButtonHandler {
    private:
        Logger* logger;
        int buttonPin;

        volatile unsigned long pressStartTime;
        volatile unsigned long pressDuration;
        volatile bool isPressHandled;

        volatile bool isPressing;
        volatile bool isReleased;

        static void IRAM_ATTR ButtonISR(void* arg);

    public:
        static const int DEBOUNCE_PERIOD_MILLIS;
        static const int LONG_PRESS_THRESHOLD_MILLIS;

        ButtonHandler(Logger* logger, int buttonPin);
        void init();
        ButtonPressType getLastPressType();
};

#endif /* BUTTON_HANDLER_H */