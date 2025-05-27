#include <IOT_TECHNION_SURFBOARD.h>
#include <vector>

// constants
int serialBaudRate = 115200;
int SDCardChipSelectPin = 5;

//********************************************* CONFIG PARAMS ***********************************
String WIFI_SSID = "";
String WIFI_PASSWORD= "";
String SENSORS_PARAMS = "[]";
//********************************************* END OF CONFIG PARAMS ****************************

void setup() {
    // run this on your esp board with an sd card to clean all files in the /sampplings directory
    Logger* logger = Logger::getInstance();
    SDCardHandler* sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
    logger->init(serialBaudRate);
    sdCardHandler->init();

    delay(500);

    sdCardHandler->deleteFile("//main.config");
    sdCardHandler->deleteFile("//unit.config");

    sdCardHandler->writeData("//main.config", String("WIFI_SSID=" + WIFI_SSID).c_str());
    sdCardHandler->writeData("//main.config", String("WIFI_PASSWORD=" + WIFI_PASSWORD).c_str());
    sdCardHandler->writeData("//main.config", String("SENSORS_PARAMS=[" + SENSORS_PARAMS + "]").c_str());
    logger->info("Config file wrote at //main.config");
    logger->info("Validating params....");

    try{
        std::map<String, String> configMap = sdCardHandler->readConfigFile("//main.config");
        String WIFI_SSID_READ = configMap["WIFI_SSID"];
        String WIFI_PASSWORD_READ = configMap["WIFI_PASSWORD"];
        std::vector<String> sensorsParams = parseSensorParams(configMap["SENSORS_PARAMS"]);
        if(WIFI_SSID == WIFI_SSID_READ && WIFI_PASSWORD == WIFI_PASSWORD_READ){
            logger->info("Validating complete!");
        } 
    }catch(exception& e){
        logger->error("Error reading config file!");
        while(true){delay(500);};
    }

}

void loop() {}

