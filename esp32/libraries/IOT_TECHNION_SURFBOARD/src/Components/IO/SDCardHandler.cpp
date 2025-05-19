#include "SDCardHandler.h"

SDCardHandler::SDCardHandler(const uint8_t SDCardChipSelectPin, Logger* logger): SDCardChipSelectPin(SDCardChipSelectPin), logger(logger){}

void SDCardHandler::init(){
    // using default sd card pins
    SPI.begin(18,19,23,SDCardChipSelectPin);
    if (!SD.begin(SDCardChipSelectPin)) {
        logger->error(F("Failed to init SD card!"));
        throw InitError();
    }
    logger->info(F("SD card initialized successfully."));
}

File SDCardHandler::open(const String &path){
    return SD.open(path);
}

bool SDCardHandler::deleteFile(const String& filePath){
    return SD.remove(filePath);
}

bool SDCardHandler::exists(const String& path) {
    return SD.exists(path);
}

fs::FS* SDCardHandler::getFS() {
    return &SD;
}

void SDCardHandler::createFolder(const String& folderName){
    // Check if the folder already exists
    if (!SD.exists(folderName)) {
        // Attempt to create the folder
        if (SD.mkdir(folderName)) {
            logger->info("Folder created successfully.");
        } else {
            logger->error("Failed to create folder.");
            throw SDCardError();
        }
    }
}

void SDCardHandler::createFile(const String& filePath){
    File file = SD.open(filePath, FILE_WRITE);
    if (!file) {
        logger->error(F("Failed to create file!"));
        throw SDCardError();
    }
    file.close();
}

void SDCardHandler::writeData(const String& filePath, const char *data){
    File file = SD.open(filePath, FILE_APPEND);
    if (!file) {
        logger->error(String("Failed to open file: ") + filePath);
        throw SDCardError();
    }
    file.println(data);
    file.flush();
    file.close();
}

bool SDCardHandler::deleteAllFilesInDir(const String& dirPath) {
    File dir = SD.open(dirPath);
    if (!dir || !dir.isDirectory()) {
        logger->error(String("Directory not found: ") + dirPath);
        return false;
    }

    File entry = dir.openNextFile();
    bool success = true;

    while (entry) {
        String fileName = entry.name();
        if (!entry.isDirectory()) {
            String fullPath = dirPath + "/" + fileName;
            logger->debug(String("Deleting file: ") + fullPath);
            if (!SD.remove(fullPath)) {
                logger->error(String("Failed to delete: ") + fullPath);
                success = false;
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }

    dir.close();
    return success;
}



std::map<String, String> SDCardHandler::readConfigFile(const String& filePath) {
    std::map<String, String> configMap;
    File configFile = SD.open(filePath, FILE_READ);
    if (!configFile) {
        logger->error(F("Config file not found!!"));
        throw SDCardError();
    }
    int lineNum = 1;
    while(configFile.available()){
        String line = configFile.readStringUntil('\n');
        int delimiterPos = line.indexOf('=');
        if(delimiterPos <= 0){
            logger->error(String("Invalid line ") + String(lineNum) + String(" in config file!"));
            throw InvalidConfigFile(); 
        }
        String key = line.substring(0, delimiterPos);
        key.trim();
        String value = line.substring(delimiterPos + 1);
        value.trim();
        configMap[key] = value;
        lineNum++;
    }
    configFile.close();
    return configMap;
}


vector<String> SDCardHandler::listFilesInDir(const String& dirName){
    vector<String> fileList;
    // Open the directory
    File dir = SD.open(dirName);
    if (!dir || !dir.isDirectory()) {
        Serial.println("Error: Directory does not exist or could not be opened.");
        throw SDCardError();
    }

    // Iterate through the files in the directory
    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) {
            // If it's a file, add the full path to the vector
            String filePath = dirName + "/" + entry.name();
            fileList.push_back(filePath);
        }
        entry.close(); // Close the current file to avoid resource leaks
    }

    dir.close(); // Close the directory
    return fileList;
}
