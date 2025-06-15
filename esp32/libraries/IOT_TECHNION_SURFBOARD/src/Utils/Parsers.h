#ifndef PARSERS_H 
#define PARSERS_H

#include <Arduino.h>
#include <vector>
#include "Exceptions.h"
#include "Status.h"

inline std::vector<String> parseSensorParams(const String& input) {
    if (input.length() < 2 || input.charAt(0) != '[' || input.charAt(input.length() - 1) != ']') {
        throw InvalidData();
    }

    String content = input.substring(1, input.length() - 1);
    std::vector<String> result;
    String currentParam;

    for (int i = 0; i < content.length(); i++) {
        char c = content.charAt(i);
        if (c == ',') {
            result.push_back(currentParam);
            currentParam = "";
        } else {
            currentParam += c;
        }
    }

    if (currentParam.length() > 0) {
        result.push_back(currentParam);
    }

    return result;
}

inline String mode_to_string(WirelessHandler::MODE mode){
    switch(mode){
        case WirelessHandler::MODE::OFF:
          return "OFF";
        case WirelessHandler::MODE::ESP_NOW:
          return "ESP_NOW";
        case WirelessHandler::MODE::WIFI:
          return "WIFI";  
    }
}

inline String sampler_status_to_string(SamplerStatus status){
    switch(status){
        case SamplerStatus::UNIT_STAND_BY:
          return "UNIT_STAND_BY";
        case SamplerStatus::UNIT_SAMPLING:
          return "UNIT_SAMPLING";
        case SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD:
          return "UNIT_SAMPLE_FILES_UPLOAD";
        case SamplerStatus::UNIT_ERROR:
          return "UNIT_ERROR";
    }
}



#endif /* PARSERS_H */