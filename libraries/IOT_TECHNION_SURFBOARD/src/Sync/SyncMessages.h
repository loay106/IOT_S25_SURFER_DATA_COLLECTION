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

enum ControlUnitCommand {
    START_SAMPLING, // attached TIMESTAMP
    STOP_SAMPLING,
    START_SAMPLE_FILES_UPLOAD, 
    STOP_SAMPLE_FILES_UPLOAD,
};

enum SamplingUnitStatusMessage{
    STAND_BY,
    SAMPLING,
    SAMPLE_FILES_UPLOAD,
    SAMPLE_FILES_UPLOAD_COMPLETE,
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
