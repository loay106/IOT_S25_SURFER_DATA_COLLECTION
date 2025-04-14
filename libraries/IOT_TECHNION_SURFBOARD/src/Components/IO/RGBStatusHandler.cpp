#include "RGBStatusHandler.h"
#include <Arduino.h>

const int FLICKERING_RATE = 500;

RGBStatusHandler* RGBStatusHandler::instance = nullptr;

portMUX_TYPE colorChangeMux = portMUX_INITIALIZER_UNLOCKED;

volatile RGBColors firstColor = RGBColors::NO_COLOR;
volatile RGBColors secondColor = RGBColors::NO_COLOR;
volatile bool flickerState = false;

void showColor(RGBColors color){
    switch (color){
        case RGBColors::RED:
            ledcWrite(0, 255); 
            ledcWrite(1, 0); 
            ledcWrite(2, 0);
            break;
        case RGBColors::GREEN:
            ledcWrite(0, 0); 
            ledcWrite(1, 255);
            ledcWrite(2, 0); 
            break;
        case RGBColors::BLUE:
            ledcWrite(0, 0); 
            ledcWrite(1, 0);
            ledcWrite(2, 255); 
            break;
        case RGBColors::CYAN:
            ledcWrite(0, 0); 
            ledcWrite(1, 255);
            ledcWrite(2, 255); 
            break;      
        case RGBColors::WHITE:
            ledcWrite(0, 255); 
            ledcWrite(1, 255);
            ledcWrite(2, 255); 
            break;      
        default:
            // defaulting to NO_COLOR
            ledcWrite(0, 0); 
            ledcWrite(1, 0);
            ledcWrite(2, 0); 
            break;
    }
}

void flicker(){
    flickerState = !flickerState;
    portENTER_CRITICAL_ISR(&colorChangeMux);
    if(firstColor == secondColor){
        portEXIT_CRITICAL_ISR(&colorChangeMux);
        return;
    }
    portEXIT_CRITICAL_ISR(&colorChangeMux);
    RGBColors color = flickerState ? firstColor : secondColor;
    showColor(color);
}

void RGBStatusHandler::init(int redPin, int greenPin, int bluePin) {
    ledcSetup(0, 5000, 8); // Channel 0, 5kHz, 8-bit resolution
    ledcSetup(1, 5000, 8); // Channel 1, 5kHz, 8-bit resolution
    ledcSetup(2, 5000, 8); // Channel 2, 5kHz, 8-bit resolution

    ledcAttachPin(redPin, 0);
    ledcAttachPin(greenPin, 1);
    ledcAttachPin(bluePin, 2);


    hw_timer_t* timer = timerBegin(0, 80, true); // 80 prescaler gives 1us per tick
    timerAttachInterrupt(timer, &flicker, true);
    timerAlarmWrite(timer, FLICKERING_RATE * 1000, true); // FLICKERING_RATE in milliseconds
    timerAlarmEnable(timer);
}



void RGBStatusHandler::updateColors(RGBColors first, RGBColors second){
    portENTER_CRITICAL_ISR(&colorChangeMux);
    if(firstColor != first || secondColor != second){
        firstColor = first;
        secondColor = second;
    }
    portEXIT_CRITICAL_ISR(&colorChangeMux);
    showColor(first);
}

