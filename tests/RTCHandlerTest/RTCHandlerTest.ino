#include <IOT_TECHNION_SURFBOARD.h>

RTCTimeHandler* timeHandler = nullptr;
Logger* logger = nullptr;

void setup() {
    logger = Logger::getInstance();
    logger->init(57600);
    timeHandler = new RTCTimeHandler(logger);
    timeHandler->init();
    // 1737459874
    //timeHandler->adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  string msg = "Current timestamp: " + to_string(timeHandler->getCurrentTimestamp());
  logger->info(msg);
  delay(1000);
}
