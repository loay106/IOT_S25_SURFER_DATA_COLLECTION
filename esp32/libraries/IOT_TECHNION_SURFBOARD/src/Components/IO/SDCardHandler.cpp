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

bool SDCardHandler::deleteFile(String filePath){
    return SD.remove(filePath);
}

void SDCardHandler::getFolder(String folderPath , File* root){
    *root = SD.open(folderPath);
    if (!(*root) || !(*root) .isDirectory()) {
        logger->error(F("Failed to open directory"));
        throw SDCardError();
    }
}

bool SDCardHandler::exists(String& path) {
    return SD.exists(path);
}

fs::FS* SDCardHandler::getFS() {
    return &SD;
}

void SDCardHandler::createFolder(string folderName){
    const char *folderNameCStr = folderName.c_str();
    // Check if the folder already exists
    if (!SD.exists(folderNameCStr)) {
        // Attempt to create the folder
        if (SD.mkdir(folderNameCStr)) {
            logger->info("Folder created successfully.");
        } else {
            logger->error("Failed to create folder.");
            throw SDCardError();
        }
    }
}

void SDCardHandler::createFile(string filePath){
    File file = SD.open(filePath.c_str(), FILE_WRITE);
    if (!file) {
        logger->error(F("Failed to create file!"));
        throw SDCardError();
    }
    file.close();
}

void SDCardHandler::writeData(string filePath, const char *data){
    File file = SD.open(filePath.c_str(), FILE_APPEND);
    if (!file) {
        logger->error(String("Failed to open file: ") + filePath.c_str());
        throw SDCardError();
    }
    file.println(data);
    file.flush();
    file.close();
}

bool SDCardHandler::deleteAllFilesInDir(String dirPath) {
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



std::map<string, string> SDCardHandler::readConfigFile(string filePath) {
    std::map<std::string, std::string> configMap;
    File configFile = SD.open(filePath.c_str(), FILE_READ);
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
        configMap[string(key.c_str())] = string(value.c_str());
        lineNum++;
    }
    configFile.close();
    return configMap;
}


vector<string> SDCardHandler::listFilesInDir(string dirName){
    vector<string> fileList;
    // Open the directory
    File dir = SD.open(dirName.c_str());
    if (!dir || !dir.isDirectory()) {
        Serial.println("Error: Directory does not exist or could not be opened.");
        throw SDCardError();
    }

    // Iterate through the files in the directory
    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) {
            // If it's a file, add the full path to the vector
            string filePath = dirName + "/" + string(entry.name());
            fileList.push_back(filePath);
        }
        entry.close(); // Close the current file to avoid resource leaks
    }

    dir.close(); // Close the directory
    return fileList;
}
