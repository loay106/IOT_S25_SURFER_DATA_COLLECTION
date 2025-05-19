#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Logging levels
enum class LogLevel {
    NONE,
    ERROR,
    INFO,
    DEBUG
};

class Logger {
private:
    LogLevel currentLevel;
    static Logger* instance;

    Logger() {
        currentLevel = LogLevel::INFO;
    }

public:
    Logger(const Logger&) = delete;

    static Logger* getInstance() {
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }

    void init(int serialBaudRate) {
        Serial.begin(serialBaudRate);
    }

    void setLogLevel(LogLevel level) {
        currentLevel = level;
    }

    void info(const String& message) {
        if (static_cast<int>(currentLevel) >= static_cast<int>(LogLevel::INFO)) {
            Serial.print(F("[INFO] "));
            Serial.println(message);
            Serial.flush();
        }
    }

    void error(const String& message) {
        if (static_cast<int>(currentLevel) >= static_cast<int>(LogLevel::ERROR)) {
            Serial.print(F("[ERROR] "));
            Serial.println(message);
            Serial.flush();
        }
    }

    void debug(const String& message) {
        if (static_cast<int>(currentLevel) >= static_cast<int>(LogLevel::DEBUG)) {
            Serial.print(F("[DEBUG] "));
            Serial.println(message);
            Serial.flush();
        }
    }
};

#endif /* LOGGER_H */
