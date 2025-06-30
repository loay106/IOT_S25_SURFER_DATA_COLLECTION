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

inline String system_status_to_string(SystemStatus status){
    switch(status){
        case SystemStatus::SYSTEM_STARTING:
          return "SYSTEM_STARTING";
        case SystemStatus::SYSTEM_STAND_BY:
          return "SYSTEM_STAND_BY";
        case SystemStatus::SYSTEM_STAND_BY_PARTIAL_ERROR:
          return "SYSTEM_STAND_BY_PARTIAL_ERROR";
        case SystemStatus::SYSTEM_SAMPLING:
          return "SYSTEM_SAMPLING";
        case SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR:
          return "SYSTEM_SAMPLING_PARTIAL_ERROR";
        case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD:
          return "SYSTEM_SAMPLE_FILE_UPLOAD";
        case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_WIFI_ERROR:
          return "SYSTEM_SAMPLE_FILE_UPLOAD_WIFI_ERROR";
        case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING:
          return "SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING";
        case SystemStatus::SYSTEM_ERROR:
          return "SYSTEM_ERROR";
    }
}

inline String command_to_string(ControlUnitCommand command){
    switch(command){
        case ControlUnitCommand::START_SAMPLING:
          return "START_SAMPLING";
        case ControlUnitCommand::STOP_SAMPLING:
          return "STOP_SAMPLING";
        case ControlUnitCommand::START_SAMPLE_FILES_UPLOAD:
          return "START_SAMPLE_FILES_UPLOAD";
        case ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD:
          return "STOP_SAMPLE_FILES_UPLOAD";
    }
}



#endif /* PARSERS_H */