#include "SyncMessages.h"

const int STATUS_REPORT_DELAY_MILLIS = 500;

String serializeStatusUpdateMsg(SamplingUnitStatusMessage status) {
    return String((int)status);
}

SamplingUnitStatusMessage deserializeStatusUpdateMsg(const uint8_t* msg, int len) {
    if (msg == nullptr || len <= 0) {
        throw InvalidSyncMessage();
    }

    String statusStr(reinterpret_cast<const char*>(msg), len);
    int statusValue = statusStr.toInt();

    if (statusValue < STAND_BY || statusValue > ERROR) {
        throw InvalidSyncMessage();
    }

    return static_cast<SamplingUnitStatusMessage>(statusValue);
}

String serializeCommand(const ControlUnitCommand& command, const std::map<String, String>& params) {
    String result = String((int)command);

    if (!params.empty()) {
        result += "|";
        for (const auto& param : params) {
            result += param.first + ":" + param.second + ";";
        }
        if (result.endsWith(";")) {
            result.remove(result.length() - 1);
        }
    }

    return result;
}

CommandMessage deserializeCommand(const uint8_t* msg, int len) {
    if (msg == nullptr || len <= 0) {
        throw InvalidSyncMessage();
    }

    String rawMsg(reinterpret_cast<const char*>(msg), len);
    CommandMessage commandMsg;

    int delimiterPos = rawMsg.indexOf('|');
    String commandPart = (delimiterPos == -1) ? rawMsg : rawMsg.substring(0, delimiterPos);

    int commandValue = commandPart.toInt();
    if (commandValue < START_SAMPLING || commandValue > STOP_SAMPLE_FILES_UPLOAD) {
        throw InvalidSyncMessage();
    }

    commandMsg.command = static_cast<ControlUnitCommand>(commandValue);

    if (delimiterPos != -1) {
        String paramsPart = rawMsg.substring(delimiterPos + 1);
        int start = 0;
        while (start < paramsPart.length()) {
            int end = paramsPart.indexOf(';', start);
            if (end == -1) end = paramsPart.length();

            String pair = paramsPart.substring(start, end);
            int colonPos = pair.indexOf(':');
            if (colonPos == -1) {
                throw InvalidSyncMessage();
            }

            String key = pair.substring(0, colonPos);
            String value = pair.substring(colonPos + 1);
            commandMsg.params[key] = value;

            start = end + 1;
        }
    }

    return commandMsg;
}
