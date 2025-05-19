#include <IOT_TECHNION_SURFBOARD.h>

// constants
int serialBaudRate = 57600;
int SDCardChipSelectPin = 5;
Sampler* sampler;
Logger* logger;
uint8_t CONTROL_UNIT_MAC[6] = {0xCC, 0xDB, 0xA7, 0x5A, 0x7F, 0xC0};
String macAddress = "CC:DB:A7:5A:7F:C0";
const string ssid = "";
const string password = "";

void FileUpload(void *param) {
    Sampler* samp = static_cast<Sampler*>(param);
    samp->uploadSampleFiles();
    vTaskDelete(NULL);
}

// globals

void setup() {
    logger = Logger::getInstance();
    SDCardHandler* sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
    WifiHandler* wifiHandler = new WifiHandler(ssid, password);
    CloudSyncManager* cloudSyncManager = new CloudSyncManager(logger,wifiHandler,macAddress);
    sampler = new Sampler(logger, sdCardHandler, cloudSyncManager);
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
  
    xTaskCreatePinnedToCore(
        FileUpload,      // Task function
        "FileUpload",    // Task name
        32768,        // Stack size (bytes) - 32kb
        sampler,        // Task parameters (can pass a struct or value)
        1,           // Priority (higher value = higher priority)
        NULL, // Task handle (can be NULL if not needed)
        1            // Core ID (0 or 1 for dual-core ESP32)
    );
    
    //sampler->uploadSampleFiles();

}

void loop() {
  if(sampler->getStatus() == SamplerStatus::UNIT_ERROR){
    logger->error("Failed to upload files!");
    delay(5000);
  }else if(sampler->getStatus() == SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
    logger->error("Uploading files in progress!");
    delay(100);
  }else if(sampler->getStatus() == SamplerStatus::UNIT_STAND_BY){
    logger->error("Uploading files complete!");
    delay(5000);
  }
 
}

