#include "SurfboardMainUnit.h"

// constants
uint8_t SDCardChipSelectPin = 5;

int serialBaudRate = 115200;

int RGBRedPin = 25;
int RGBGreenPin = 26;
int RGBBluePin = 27;

int buttonPin = 4;

// globals
SurfboardMainUnit* mainUnit;
Logger* logger;
RGBStatusHandler* statusLighthandler;

uint8_t samplingUnitsMacAddresses[1][6] =  {
    {0xCC, 0xDB, 0xA7, 0x5A, 0x7F, 0xC0}, // Loay's esp testing board
   // {0xA8, 0x42, 0xE3, 0x45, 0x94, 0x68}, // Mousa's esp board
   // {0x0C, 0xB8, 0x15, 0x77, 0x84, 0x64} // Shada's esp board
};

void setup() {
    logger = Logger::getInstance();
    logger->init(serialBaudRate);
    logger->setLogLevel(LogLevel::DEBUG);

    logger->info("System init starting...");

    statusLighthandler = RGBStatusHandler::getInstance();
    try{
        statusLighthandler->init(RGBRedPin, RGBGreenPin, RGBBluePin);
    }catch(...){
        logger->error("Failed to start LED! Check your wiring!");
        while(true){delay(500);};
    }

    SDCardHandler* sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
    try{
        sdCardHandler->init();
    }catch(InitError& err){
        logger->error("SD Card init error! Check your SD card wiring!");
        while(true){delay(500);};
    }

    string WIFI_SSID = "";
    string WIFI_PASSWORD = "";
    vector<string> sensorsParams;

    try{
        std::map<string, string> configMap = sdCardHandler->readConfigFile("//unit.config");
        WIFI_SSID = configMap["WIFI_SSID"];
        WIFI_PASSWORD = configMap["WIFI_PASSWORD"];
        sensorsParams = parseSensorParams(configMap["SENSORS_PARAMS"]);
    }catch(exception& e){
        logger->error("Error reading config file!");
        while(true){delay(500);};
    }

    WifiHandler* wifiHandler = new WifiHandler(logger, WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());

    ControlUnitSyncManager* syncManager = ControlUnitSyncManager::getInstance();
    RTCTimeHandler* timeHandler = new RTCTimeHandler(logger);
    ButtonHandler* buttonHandler = new ButtonHandler(logger, buttonPin);

    Sampler* sampler = new Sampler(logger, sdCardHandler);
    String ownMacAddress = wifiHandler->getMacAddress().c_char();
    DataCollectorServer* server = new DataCollectorServer(sdCardHandler, ownMacAddress, true);
    mainUnit = new SurfboardMainUnit(syncManager, timeHandler, statusLighthandler, buttonHandler, logger, sampler, sdCardHandler, WIFI_SSID, WIFI_PASSWORD, server);

    // declare sensors here....
    Mock_HX711* mock_force_0 = new Mock_HX711(logger,sdCardHandler, 1000);
    Mock_HX711* mock_force_1 = new Mock_HX711(logger,sdCardHandler, 1000);

    try{
        // don't change the order of the init
        syncManager->init(samplingUnitsMacAddresses, 0, 0);
        timeHandler->init();
        buttonHandler->init();
        sampler->init();
        mainUnit->init(samplingUnitsMacAddresses, 0);

        // init sensors here..
        // you can pass params from the config file
        // sensor[i] should have the param in sensorsParams[i]
        mock_force_0->init();
        mock_force_1->init();
    }catch(InitError& err){
        logger->error("Init error! Check your wiring!");
        while(true){delay(500);};
    }

    // add sensors here....
    mainUnit->addSensor(mock_force_0);
    mainUnit->addSensor(mock_force_1);

    while(true){
      try{
          syncManager->connect();
      }catch(ESPNowSyncError& err){
          logger->info("Failed to connect to esp-now!");
          while(true){delay(500);};
      }
    }

    logger->info("System init complete!");
}

void loop() {
    try{  
        mainUnit->handleButtonPress();
        mainUnit->readStatusUpdateMessages();
        mainUnit->loopDiscoverDisconnected();
        SystemStatus status = mainUnit->getStatus();
        switch(status){
          case SystemStatus::SYSTEM_STAND_BY:
          case SystemStatus::SYSTEM_STARTING:
            mainUnit->loopStandby();
            delay(10);
            break;
          case SystemStatus::SYSTEM_SAMPLING:
          case SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR:
            mainUnit->loopSampling();
            break;
          case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD:
          case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_PARTIAL_ERROR:
            mainUnit->loopFileUpload();
            delay(5);
            break;
          case SystemStatus::SYSTEM_ERROR: 
            statusLighthandler->updateColors(RGBColors::RED, RGBColors::RED);
            delay(500);
            break;
        }
    }catch(...){
        logger->error("An unexpected error occured! Trying to recover...");
        statusLighthandler->updateColors(RGBColors::RED, RGBColors::NO_COLOR);
        delay(200); // give some time for the system to recover
    }
}

