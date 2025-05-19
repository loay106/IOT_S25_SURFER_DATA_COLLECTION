#include "Logger.h"

Logger* Logger::instance = nullptr;

void Logger::logMessage(LogLevel level, string prefix, string message){
    if (static_cast<int>(level) <= static_cast<int>(currentLevel)) {
        string formattedMessage = "[" + prefix + "] " + message;
        Serial.println(String(formattedMessage.c_str()));
        Serial.flush();
    }
}