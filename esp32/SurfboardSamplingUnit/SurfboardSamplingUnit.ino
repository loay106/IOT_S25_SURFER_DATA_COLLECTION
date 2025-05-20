#include "SurfboardSamplingUnit.h"

// constants
int serialBaudRate = 115200;
int SDCardChipSelectPin = 5;
uint8_t CONTROL_UNIT_MAC[6] = {0xA8, 0x42, 0xE3, 0x46, 0xE0, 0x64};

int doutPin = 12;
 int sckPin = 13;

// globals
SurfboardSamplingUnit* samplingUnit; 

void setup() {
    Logger* logger = Logger::getInstance();
    logger->init(serialBaudRate);
    logger->setLogLevel(LogLevel::DEBUG);
    SDCardHandler* sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
    try{
        sdCardHandler->init();
    }catch(InitError& err){
        logger->error("SD Card init error! Check your SD card wiring!");
        while(true){delay(500);};
    }

    String WIFI_SSID = "";
    String WIFI_PASSWORD = "";
    vector<String> sensorsParams;
    int WIFI_ESP_NOW_CHANNEL = 0;

    try{
        std::map<String, String> configMap = sdCardHandler->readConfigFile("/unit.config");
        WIFI_SSID = configMap["WIFI_SSID"];
        WIFI_PASSWORD = configMap["WIFI_PASSWORD"];
        sensorsParams = parseSensorParams(configMap["SENSORS_PARAMS"]);
    }catch(exception& e){
        logger->error("Error reading config file! Place unit.config in your sd card top level with the required configs!");
        while(true){delay(500);};
    }


    WifiHandler* wifiHandler = new WifiHandler(WIFI_SSID, WIFI_PASSWORD);
    while(true){
        try{
            logger->info("Checking wifi connection...");
            wifiHandler->init();
            wifiHandler->connect();
            logger->info("Wifi connection established!");
            WIFI_ESP_NOW_CHANNEL = wifiHandler->getChannel();
            logger->info("Setting ESP now channel to " + String(WIFI_ESP_NOW_CHANNEL));
            wifiHandler->disconnect();
            logger->info("Wifi disconnected!");
            break;
        }catch(...){
            logger->error("Wifi connection failed! Please check your wifi ssid and password!");
            delay(3000);
        }
    }

    CloudSyncManager* cloudSyncManager = new CloudSyncManager(logger, wifiHandler, wifiHandler->getMacAddress());
    Sampler* sampler = new Sampler(logger, sdCardHandler, cloudSyncManager);

    SamplingUnitSyncManager* syncManager = SamplingUnitSyncManager::getInstance();
    samplingUnit = new SurfboardSamplingUnit(syncManager,sdCardHandler,sampler, logger);

    // declare sensors here....
    //Force_HX711* sensor0 = new Force_HX711(logger,sdCardHandler,stoi(sensorsParams[0]) ,doutPin,sckPin);
    //IMU_BNO080* sensor1 = new IMU_BNO080(logger, sdCardHandler,stoi(sensorsParams[1]));
    Mock_HX711* mock_force_2 = new Mock_HX711(logger,sdCardHandler, 1000);
    Mock_HX711* mock_force_3 = new Mock_HX711(logger,sdCardHandler, 1000);
    Mock_HX711* mock_force_4 = new Mock_HX711(logger,sdCardHandler, 1000);

    try{
        // don't change the order of the init
        logger->init(serialBaudRate);
        syncManager->init(CONTROL_UNIT_MAC, WIFI_ESP_NOW_CHANNEL);
        sampler->init();
        // init sensors here..
       // sensor0->init();
        //sensor1->init();
        mock_force_2->init();
        mock_force_3->init();
        mock_force_4->init();
    }catch(InitError& err){
        while(true){
            // don't proceed, there's a wiring error!
            logger->error("Init error! Check your wiring!");
            delay(100);
        }
    }

    // add sensors here....
   // samplingUnit->addSensor(sensor0);
   // samplingUnit->addSensor(sensor1);
    samplingUnit->addSensor(mock_force_2);
    samplingUnit->addSensor(mock_force_3);
    samplingUnit->addSensor(mock_force_4);

    try{
        syncManager->connect();
    }catch(ESPNowSyncError& err){
        while(true){delay(500);};
    }
    
    logger->info("Unit setup complete!");
}

void loop() {
    samplingUnit->handleNextCommand();
    SamplerStatus status = samplingUnit->getStatus();
    switch(status){
        case UNIT_STAND_BY:
            delay(5);
            break;
        case UNIT_ERROR:
            delay(5);
            break;
        case UNIT_SAMPLING:
            samplingUnit->loopSampling();
            delay(5);
            break;
        case UNIT_SAMPLE_FILES_UPLOAD:
            samplingUnit->loopFileUpload();
            delay(10);
            break;
    }

    switch (status)
    {
        case UNIT_STAND_BY:
            samplingUnit->reportStatus(SamplingUnitStatusMessage::STAND_BY);
            break;

        case UNIT_SAMPLING:
            samplingUnit->reportStatus(SamplingUnitStatusMessage::SAMPLING);
            break;

        case UNIT_ERROR:
            samplingUnit->reportStatus(SamplingUnitStatusMessage::ERROR);
            break;

        case UNIT_SAMPLE_FILES_UPLOAD: 
            samplingUnit->reportStatus(SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD);
            break;
        
        default:
            break;
    }

}
