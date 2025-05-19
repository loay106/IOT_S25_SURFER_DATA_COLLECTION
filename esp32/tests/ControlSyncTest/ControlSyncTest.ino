#include <IOT_TECHNION_SURFBOARD.h>

uint8_t samplingUnitsMacAddresses[1][6] =  {
    {0xA8, 0x42, 0xE3, 0x45, 0x94, 0x68} //MOUSA ESP32 MAC
};

std::map<string, string> commandParams = {
    {"pin", "5"},       
    {"state", "HIGH"},
    {"brightness","255"},
    {"color", "red"}
};


const int serialBaudRate = 115200;
Logger* logger;
ControlUnitSyncManager* controlSyncManager;


//////////////////Print Functions/////////////////////////

void printStatusUpdateMessage(StatusUpdateMessage msg) {
    logger->info("status update from : " + macToString(msg.from));
    switch (msg.status) {
        case UNIT_STAND_BY:
            logger->info("UNIT_STAND_BY");
            break;
        case UNIT_SAMPLING:
            logger->info("UNIT_SAMPLING");
            break;
        case UNIT_ERROR:
            logger->info("UNIT_ERROR");
            break;
        case UNIT_SAMPLE_FILES_UPLOAD:
            logger->info("UNIT_SAMPLE_FILES_UPLOAD");
            break;
        default:
            logger->error("Unknown Command");
            break;
    }
}
//////////////////////////////////////////////////////////


void setup() {
    logger = Logger::getInstance();
    controlSyncManager = ControlUnitSyncManager::getInstance();
    try{
        logger->init(serialBaudRate);    
        controlSyncManager->init(samplingUnitsMacAddresses,1);
    }
    catch(InitError& err){
      while(true){
          // don't proceed, there's a wiring error!
          logger->error("Init error! Check your wiring!");
          delay(100);
      }
    }
    controlSyncManager->connect();
    delay(1000);

}


void loop()
{
    controlSyncManager->broadcastCommand(START_SAMPLING,commandParams);
    delay(500);
    StatusUpdateMessage unit_status = controlSyncManager->popStatusUpdateMessage();
    printStatusUpdateMessage(unit_status);
    delay(500); 
}