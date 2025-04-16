#ifndef SDCARD_HANDLER_H 
#define SDCARD_HANDLER_H

using namespace std;
#include <cstdint>
#include <string>
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

        class SDCardFileReader{
            private:
                File file;
            public:
                SDCardFileReader(File file);
                String readNextLine();
                void close();
        };

        void init();
        void getFolder(String folderPath , File* root);
        File open(const String& path) {
            return SD.open(path);
        }
        void createFolder(string folderName);
        void createFile(string filePath);
        void deleteFile(String filePath);
        void writeData(string filePath,const char* data);
        bool exists(String& path);
        fs::FS* getFS();
        String calculateMD5(String& path);

        SDCardHandler::SDCardFileReader readFile(string filePath);

        std::map<string, string> readConfigFile(string filePath);

        vector<string> listFilesInDir(string dirName);
};


#endif /* SDCARD_HANDLER_H */