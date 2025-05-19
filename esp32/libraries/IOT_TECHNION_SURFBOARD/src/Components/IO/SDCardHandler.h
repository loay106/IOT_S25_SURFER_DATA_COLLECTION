#ifndef SDCARD_HANDLER_H 
#define SDCARD_HANDLER_H

using namespace std;
#include <cstdint>
#include <vector>
#include <map>
#include <SD.h>
#include <Arduino.h>
#include <Wire.h>
#include <FS.h>

#include "../../Utils/Exceptions.h"
#include "Logger.h"

class SDCardHandler{
    private:
        Logger* logger;
        uint8_t SDCardChipSelectPin;
    public:
        SDCardHandler(const uint8_t SDCardChipSelectPin, Logger* logger);

        void init();
        File open(const String& path);
        void createFolder(const String& folderName);
        void createFile(const String& filePath);
        bool deleteFile(const String& filePath);
        bool deleteAllFilesInDir(const String& dirPath);
        void writeData(const String& filePath,const char* data);
        bool exists(const String& path);
        fs::FS* getFS();

        std::map<String, String> readConfigFile(const String& filePath);

        vector<String> listFilesInDir(const String& dirName);
};


#endif /* SDCARD_HANDLER_H */