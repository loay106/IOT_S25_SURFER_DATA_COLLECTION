#include <IOT_TECHNION_SURFBOARD.h>

uint8_t SDCardChipSelectPin = 5;
const int serialBaudRate = 57600;

Logger* logger;
SDCardHandler* sdCardHandler;
Sampler* sampler;


void setup() {
  logger = Logger::getInstance();
  logger->init(serialBaudRate);
  sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
  try{
      sdCardHandler->init();
  }catch(InitError& err){
      while(true){delay(500);};
  }

  try{
      vector<string> files = sdCardHandler->listFilesInDir("/");
      for(string f: files){
        logger->info(f);
      }
  }catch(exception& e){
    while(true){delay(500);};
  }
  try{
    std::map<string, string> configMap = sdCardHandler->readConfigFile("//main_unit.config");
    for (auto const& item : configMap){
      string message = "Key=" + item.first + " Value=" + item.second;
      logger->info(message);
    }
  }catch(exception& e){
    while(true){delay(500);};
  }


}

void loop() {}



