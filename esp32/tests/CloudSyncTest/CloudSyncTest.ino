#include <IOT_TECHNION_SURFBOARD.h>

// constants
int serialBaudRate = 57600;
int SDCardChipSelectPin = 5;
uint8_t CONTROL_UNIT_MAC[6] = {0xCC, 0xDB, 0xA7, 0x5A, 0x7F, 0xC0};
String macAddress = "CC:DB:A7:5A:7F:C0";
const string ssid = "";
const string password = "";

void setup() {
    Logger* logger = Logger::getInstance();
    SDCardHandler* sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
    WifiHandler* wifiHandler = new WifiHandler(ssid, password);
    CloudSyncManager* cloudSyncManager = new CloudSyncManager(logger,wifiHandler,macAddress);
    Sampler* sampler = new Sampler(logger, sdCardHandler, cloudSyncManager);
    Force_FAKE* fake_force_0 = new Force_FAKE(logger,sdCardHandler);
    Force_FAKE* fake_force_1 = new Force_FAKE(logger,sdCardHandler);

    logger->init(serialBaudRate);
    wifiHandler->init();
    cloudSyncManager->init();
    sdCardHandler->init();
    sampler->init();

    sampler->addSensor(fake_force_0);
    sampler->addSensor(fake_force_1);

    // clean samplings dir first
    vector<string> files = sdCardHandler->listFilesInDir("/samplings");
    logger->info("cleaning " + to_string(files.size()) + " from samplings dir!");
    for(string f: files){
      sdCardHandler->deleteFile(f.c_str());
    }
    logger->info("cleaning complete!");

    unsigned long startTimeMillis = millis();
    sampler->startSampling(1738229620);

    while(millis() - startTimeMillis < 3000){
      sampler->writeSensorsData();
      delay(5);
    }
    sampler->stopSampling();
    sampler->uploadSampleFiles();
}

void loop() {}

