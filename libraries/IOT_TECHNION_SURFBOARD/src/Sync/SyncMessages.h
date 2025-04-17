#ifndef SYNC_MESSAGES_H 
#define SYNC_MESSAGES_H

#include <map>
#include <string>
#include <cstdint>
#include <vector>
#include <sstream>
#include <stdexcept>

#include "../Utils/Status.h"
#include "../Utils/Exceptions.h"

using namespace std;

extern const int STATUS_REPORT_DELAY_MILLIS;

enum ControlUnitCommand {
    START_SAMPLING, // attached TIMESTAMP, done with esp-now
    STOP_SAMPLING, // done with esp-now
    START_SAMPLE_FILES_UPLOAD, // attached wifi_ssid and password, done with wifi
    STOP_SAMPLE_FILES_UPLOAD, // done with wifi
};

enum SamplingUnitStatusMessage{
    STAND_BY,
    SAMPLING,
    SAMPLE_FILES_UPLOAD,
    ERROR,
};

typedef struct CommandMessage {
    ControlUnitCommand command;
    std::map<string, string> params;    
} CommandMessage;

typedef struct StatusUpdateMessage{
    uint8_t from[6]; 
    SamplingUnitStatusMessage status;
} StatusUpdateMessage;

string serializeStatusUpdateMsg(SamplingUnitStatusMessage status);
SamplingUnitStatusMessage deserializeStatusUpdateMsg(const uint8_t* msg, int len);

string serializeCommand(const ControlUnitCommand& command, const std::map<string,string>& params);
CommandMessage deserializeCommand(const uint8_t* msg, int len);

#endif /* SYNC_MESSAGES_H */
