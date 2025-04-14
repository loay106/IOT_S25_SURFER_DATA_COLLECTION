#ifndef RTC_TIME_HANDLER_H 
#define RTC_TIME_HANDLER_H

#include "RTClib.h"
#include "Logger.h"
#include <Arduino.h>

class RTCTimeHandler{
    private:
        Logger* logger;
        RTC_DS3231 rtc;
    public:
        RTCTimeHandler(Logger* logger): logger(logger){};

        void init(){
            if (!rtc.begin()) {
                logger->error("Couldn't find RTC device. Check the wiring");
                throw InitError();
            }
        }

        int getCurrentTimestamp(){
            DateTime time = rtc.now();
            return time.unixtime();
        }
        
        void adjust(DateTime current){
            rtc.adjust(current);
        }
};


#endif /* RTC_TIME_HANDLER_H */