#include <IOT_TECHNION_SURFBOARD.h>

uint8_t controlUnitMacAddress[6]={
        0x10, 0x06, 0x1C, 0x86, 0x00, 0x18
  };//MOUSA ESP32 MAC
const int serialBaudRate = 115200;
Logger* logger;
SamplingUnitSyncManager* samplingSyncManager;


//////////////////Print Functions/////////////////////////

string getMap(const std::map<string, string>& myMap) {
    string content="(";
    auto it = myMap.begin();
    while (it != myMap.end()) {
        content += it->first + " : " + it->second;
        ++it;
        if (it != myMap.end()) {
            content += " , ";
        }
    }
    content+=")";
    return content;
}


string getControlUnitCommand(ControlUnitCommand cmd) {
    switch (cmd) {
        case START_SAMPLING:
            return "START_SAMPLING";
            break;
        case STOP_SAMPLING:
            return "STOP_SAMPLING";
            break;
        case START_SAMPLE_FILES_UPLOAD:
            return "START_SAMPLE_FILES_UPLOAD";
            break;
        case STOP_SAMPLE_FILES_UPLOAD:
            return "STOP_SAMPLE_FILES_UPLOAD";
            break;
        default:
            return "Unknown Command";
            break;
    }
}

void printMessage(CommandMessage message)
{
    string cmd = getControlUnitCommand(message.command);
    string map = getMap(message.params);
    logger->info(cmd + " | " + map);
}

//////////////////////////////////////////////////////////


void setup() {
    logger = Logger::getInstance();
    samplingSyncManager = SamplingUnitSyncManager::getInstance();
    try{
        logger->init(serialBaudRate);    
        samplingSyncManager->init(controlUnitMacAddress);

    }
    catch(InitError& err){
      while(true){
          // don't proceed, there's a wiring error!
          logger->error("Init error! Check your wiring!");
          delay(100);
      }
    }
    samplingSyncManager->connect();
    delay(1000);

}


void loop()
{
    try{
        CommandMessage message = samplingSyncManager->getNextCommand();
        printMessage(message);
        samplingSyncManager->reportStatus(SAMPLING);
    }
    catch(NotReadyError& err){
        logger->error("No Commands");
    }
    delay(500);
    
}