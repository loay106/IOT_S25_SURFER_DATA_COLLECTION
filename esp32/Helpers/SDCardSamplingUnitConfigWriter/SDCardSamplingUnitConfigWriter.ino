#include <IOT_TECHNION_SURFBOARD.h>
#include <vector>

// constants
int serialBaudRate = 115200;
int SDCardChipSelectPin = 5;

//********************************************* CONFIG PARAMS ***********************************
String SENSORS_PARAMS = "[]";
//********************************************* END OF CONFIG PARAMS ****************************

void setup() {
    // run this on your esp board with an sd card to clean all files in the /sampplings directory
    Logger* logger = Logger::getInstance();
    SDCardHandler* sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
    logger->init(serialBaudRate);
    sdCardHandler->init();

    delay(500);

    sdCardHandler->deleteFile("//unit.config");

    sdCardHandler->writeData("//unit.config", String("SENSORS_PARAMS=[" + SENSORS_PARAMS + "]").c_str());
    logger->info("Config file wrote at //unit.config");
    logger->info("Validating params....");

    try{
        std::map<String, String> configMap = sdCardHandler->readConfigFile("//unit.config");
        std::vector<String> sensorsParams = parseSensorParams(configMap["SENSORS_PARAMS"]);
    }catch(exception& e){
        logger->error("Error reading config file!");
        while(true){delay(500);};
    }

}

void loop() {}

