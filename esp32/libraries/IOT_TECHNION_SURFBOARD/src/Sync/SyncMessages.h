#ifndef SYNC_MESSAGES_H 
#define SYNC_MESSAGES_H

#include <Arduino.h>
#include <map>

#include "../Utils/Status.h"
#include "../Utils/Exceptions.h"

extern const int STATUS_REPORT_DELAY_MILLIS;

enum ControlUnitCommand {
    START_SAMPLING,
    STOP_SAMPLING,
    START_SAMPLE_FILES_UPLOAD,
    STOP_SAMPLE_FILES_UPLOAD,
};

enum SamplingUnitStatusMessage {
    STAND_BY,
    SAMPLING,
    SAMPLE_FILES_UPLOAD,
    ERROR,
};

typedef struct CommandMessage {
    ControlUnitCommand command;
    std::map<String, String> params;
} CommandMessage;

typedef struct StatusUpdateMessage {
    uint8_t from[6];
    SamplingUnitStatusMessage status;
} StatusUpdateMessage;

String serializeStatusUpdateMsg(SamplingUnitStatusMessage status);
SamplingUnitStatusMessage deserializeStatusUpdateMsg(const uint8_t* msg, int len);

String serializeCommand(const ControlUnitCommand& command, const std::map<String, String>& params);
CommandMessage deserializeCommand(const uint8_t* msg, int len);

#endif /* SYNC_MESSAGES_H */
