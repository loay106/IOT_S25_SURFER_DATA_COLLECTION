#include "ButtonHandler.h"

const int ButtonHandler::DEBOUNCE_PERIOD_MILLIS = 50;
const int ButtonHandler::LONG_PRESS_THRESHOLD_MILLIS = 1000;

ButtonHandler::ButtonHandler(Logger* logger, int buttonPin)
    : logger(logger), buttonPin(buttonPin), pressStartTime(0), pressDuration(0), 
      isPressHandled(false), isPressing(false), isReleased(false) {}

void IRAM_ATTR ButtonHandler::ButtonISR(void* arg) {
    ButtonHandler* handler = static_cast<ButtonHandler*>(arg);
    unsigned long currentTime = millis();

    if (digitalRead(handler->buttonPin) == LOW) { // Button pressed
        if (!handler->isPressing) {
            handler->isPressing = true;
            handler->isReleased = false;
            handler->pressStartTime = currentTime;
            handler->isPressHandled = false;
        }
    } else { // Button released
        if (handler->isPressing && (currentTime - handler->pressStartTime) >= DEBOUNCE_PERIOD_MILLIS) {
            handler->isPressing = false;
            handler->isReleased = true;
            handler->pressDuration = currentTime - handler->pressStartTime;
        }
    }
}

void ButtonHandler::init() {
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(buttonPin), ButtonISR, this, CHANGE);
    logger->info("ButtonHandler initialized for pin " + std::to_string(buttonPin));
}

ButtonPressType ButtonHandler::getLastPressType() {
    if (!isReleased || isPressHandled) {
        return NO_PRESS;
    }

    isPressHandled = true;
    isReleased = false;

    if (pressDuration >= LONG_PRESS_THRESHOLD_MILLIS) {
       // logger->debug("Detected long press on pin " + std::to_string(buttonPin));
        return LONG_PRESS;
    } else if (pressDuration >= DEBOUNCE_PERIOD_MILLIS) {
       // logger->debug("Detected soft press on pin " + std::to_string(buttonPin));
        return SOFT_PRESS;
    }

    //logger->debug("Press duration too short. Ignoring.");
    return NO_PRESS;
}