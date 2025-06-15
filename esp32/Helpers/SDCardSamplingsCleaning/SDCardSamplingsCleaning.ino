#include <IOT_TECHNION_SURFBOARD.h>

// constants
int serialBaudRate = 115200;
int SDCardChipSelectPin = 5;

void setup() {
    // run this on your esp board with an sd card to clean all files in the /sampplings directory
    Logger* logger = Logger::getInstance();
    SDCardHandler* sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
    logger->init(serialBaudRate);
    sdCardHandler->init();

    delay(500);

    // clean samplings dir first
    vector<string> files = sdCardHandler->listFilesInDir("/samplings");
    logger->info("cleaning " + to_string(files.size()) + " from samplings dir!");
    for(string f: files){
      sdCardHandler->deleteFile(f.c_str());
    }
    logger->info("cleaning complete!");
}

void loop() {}

