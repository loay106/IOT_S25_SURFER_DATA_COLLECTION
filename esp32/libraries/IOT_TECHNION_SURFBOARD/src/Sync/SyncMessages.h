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

typedef struct CommandMessage {
    ControlUnitCommand command;
    std::map<String, String> params;
} CommandMessage;

typedef struct StatusUpdateMessage {
    uint8_t from[6];
    SamplerStatus status;
} StatusUpdateMessage;

String serializeStatusUpdateMsg(SamplerStatus status);
SamplerStatus deserializeStatusUpdateMsg(const uint8_t* msg, int len);

String serializeCommand(const ControlUnitCommand& command, const std::map<String, String>& params);
CommandMessage deserializeCommand(const uint8_t* msg, int len);

#endif /* SYNC_MESSAGES_H */
