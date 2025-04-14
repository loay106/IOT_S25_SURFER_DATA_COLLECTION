#include "SDCardHandler.h"

SDCardHandler::SDCardHandler(const uint8_t SDCardChipSelectPin, Logger* logger): SDCardChipSelectPin(SDCardChipSelectPin), logger(logger){}

void SDCardHandler::init(){
    // using default sd card pins
    SPI.begin(18,19,23,SDCardChipSelectPin);
    if (!SD.begin(SDCardChipSelectPin)) {
        logger->error("Failed to init SD card!");
        throw InitError();
    }
    logger->info("SD card initialized successfully.");
}

void SDCardHandler::deleteFile(String filePath){
    SD.remove(filePath);
}

void SDCardHandler::getFolder(String folderPath , File* root){
    *root = SD.open(folderPath);
    if (!(*root) || !(*root) .isDirectory()) {
        logger->error("Failed to open directory");
        throw SDCardError();
    }
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
        logger->error("Failed to create file!");
        throw SDCardError();
    }
    file.close();
}

void SDCardHandler::writeData(string filePath, const char *data){
    File file = SD.open(filePath.c_str(), FILE_APPEND);
    if (!file) {
        logger->error("Failed to open file: " + filePath);
        throw SDCardError();
    }
    file.println(data);
    file.flush();
    file.close();
}

SDCardHandler::SDCardFileReader SDCardHandler::readFile(string filePath){
    File file = SD.open(filePath.c_str(), FILE_READ);
    if (!file) {
        throw SDCardError();
    }
    SDCardFileReader reader = SDCardFileReader(file);
    return reader;
}

#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::map<string, string> SDCardHandler::readConfigFile(string filePath) {
    std::map<std::string, std::string> configMap;
    File configFile = SD.open(filePath.c_str(), FILE_READ);
    if (!configFile) {
        logger->error("Config file not found!!");
        throw SDCardError();
    }
    int lineNum = 1;
    while(configFile.available()){
        String line = configFile.readStringUntil('\n');
        int delimiterPos = line.indexOf('=');
        if(delimiterPos <= 0){
            string message = "Invalid line " + to_string(lineNum) + " in config file!";
            logger->error(message);
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

SDCardHandler::SDCardFileReader::SDCardFileReader(File file){
    this->file = file;
}

String SDCardHandler::SDCardFileReader::readNextLine(){
    if(!file){
        throw SDCardError();
    }
    if(file.available()){
        String line = file.readStringUntil('\n');
        return line;
    }else{
        throw EndOfFileError();
    }
}

void SDCardHandler::SDCardFileReader::close(){
    file.close();
}
