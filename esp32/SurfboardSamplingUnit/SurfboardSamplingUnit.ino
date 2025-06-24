#include "SurfboardSamplingUnit.h"

// ******************************* GLOBAL VARIABLES *************************************
SurfboardSamplingUnit* samplingUnit; 
Logger* logger;
SDCardHandler* sdCardHandler;
// ******************************* END OF GLOBAL VARIABLES ******************************

// ******************************* UNIT CONFIG - EDIT HERE ******************************
uint8_t CONTROL_UNIT_MAC[6] = {0xA8, 0x42, 0xE3, 0x46, 0xE0, 0x64};

void addSensors(vector<String> sensorsParams){
    /*  add sensors here...
        you can pass params from the config file in the sd card
        sensor[i] should have the param in sensorsParams[i]
    */
    Mock_HX711* mock_force_0 = new Mock_HX711(logger,sdCardHandler, 1000);
    Mock_HX711* mock_force_1 = new Mock_HX711(logger,sdCardHandler, 1000);
    Mock_HX711* mock_force_2 = new Mock_HX711(logger,sdCardHandler, 1000);
    IMU_BNO080* imu_sensor = new IMU_BNO080(logger, sdCardHandler, 1000);
    Force_HX711* force_sensor = new Force_HX711(logger, sdCardHandler, 100, 12, 13);

    mock_force_0->init();
    mock_force_1->init();
    mock_force_2->init();
    imu_sensor->init();
    force_sensor->init();

    samplingUnit->addSensor(mock_force_0);
    samplingUnit->addSensor(mock_force_1);
    samplingUnit->addSensor(mock_force_2);
    samplingUnit->addSensor(imu_sensor);
    samplingUnit->addSensor(force_sensor);
    
}
// ******************************* END OF UNIT CONFIG ***********************************

// ******************************* PARAMETERS *******************************************
int doutPin = 12;
int sckPin = 13;
int serialBaudRate = 115200;
int SDCardChipSelectPin = 5;
int ESP_NOW_CHANNEL = 0;
// ******************************* END PARAMETERS ***************************************


void setup() {
    logger = Logger::getInstance();
    logger->init(serialBaudRate);
    logger->setLogLevel(LogLevel::DEBUG);
    delay(1000);

    logger->info("Sampling unit init starting...");

    sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
    try{
        sdCardHandler->init();
    }catch(InitError& err){
        logger->error("SD Card init error! Check your SD card wiring!");
        while(true){delay(500);};
    }

    vector<String> sensorsParams;
    try{
        std::map<String, String> configMap = sdCardHandler->readConfigFile("//unit.config");
        sensorsParams = parseSensorParams(configMap["SENSORS_PARAMS"]);
    }catch(exception& e){
        logger->error("Error reading config file!");
        while(true){delay(500);};
    }

    Sampler* sampler = new Sampler(logger, sdCardHandler);
    vector<uint8_t*> mainUnitsMacAddress;
    mainUnitsMacAddress.push_back(CONTROL_UNIT_MAC);
    WirelessHandler* wirelessHandler = new WirelessHandler(logger, ESP_NOW_CHANNEL, mainUnitsMacAddress, SamplingUnitSyncManager::onDataReceivedCallback);
    SamplingUnitSyncManager* syncManager = SamplingUnitSyncManager::getInstance();
    String ownMacAddress = wirelessHandler->getMacAddress();
    DataCollectorServer* server = new DataCollectorServer(sdCardHandler, ownMacAddress, false);
    samplingUnit = new SurfboardSamplingUnit(wirelessHandler, syncManager,sdCardHandler,sampler, logger, server);

    try{
        // don't change the order of the init
        logger->init(serialBaudRate);
        addSensors(sensorsParams);
        sampler->init();
        syncManager->init(CONTROL_UNIT_MAC);
    }catch(InitError& err){
        while(true){
            // don't proceed, there's a wiring error!
            logger->error("Init error! Check your wiring!");
            delay(100);
        }
    }    

    if(wirelessHandler->getCurrentMode() == WirelessHandler::MODE::OFF){
      wirelessHandler->switchToESPNow();
      while(!wirelessHandler->isConnected()){
          wirelessHandler->loop();
      }
    }
    logger->info("Unit setup complete!");
}

void loop() {
    samplingUnit->loopComponents();
    samplingUnit->handleNextCommand();
    SamplerStatus status = samplingUnit->getStatus();
    switch(status){
        case UNIT_STAND_BY:
            samplingUnit->loopStandBy();
            samplingUnit->reportStatus(false);
            break;
        case UNIT_SAMPLING:
            samplingUnit->loopSampling();
            samplingUnit->reportStatus(false);
            break;
        case UNIT_SAMPLE_FILES_UPLOAD:
            samplingUnit->loopFileUpload();
            break;
        case UNIT_ERROR:
            delay(10);
            break;
    }
}
