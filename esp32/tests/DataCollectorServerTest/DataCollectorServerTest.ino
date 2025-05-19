// esp32_loop_code.ino
#include <IOT_TECHNION_SURFBOARD.h>

uint8_t SDCardChipSelectPin = 5;
const int serialBaudRate = 115200;

// Create instances
WifiHandler* wifi;
Logger* logger;
SDCardHandler* sdCardHandler; 
DataCollectorServer* server;

// Replace with your Wi-Fi credentials
const char* ssid = "BEZEQINT-F2A4-2.4G";
const char* password = "fEuw4oZdvCJ4L";

void setup() {
  logger = Logger::getInstance();
  logger->init(serialBaudRate);
  logger->setLogLevel(LogLevel::DEBUG);
  logger->info("Booting ESP32 test device");

  sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
  sdCardHandler->init();

  // Connect to Wi-Fi
  wifi = new WifiHandler(logger, ssid, password);
  wifi->connect();

  // Start uploader with MAC as identifier
  String macId = "CC_D8_A7_5A_7F_CO";
  server = new DataCollectorServer(sdCardHandler, macId, false);
  server->begin();

}

void loop() {
  if(!wifi->isConnected()){
    wifi->reconnect();
  }else{
    server->loop();
    //logger->info(wifi->getIP().c_str());
    delay(500);
  }
}
