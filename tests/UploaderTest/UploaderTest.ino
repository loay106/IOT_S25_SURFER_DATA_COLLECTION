// esp32_loop_code.ino
#include <IOT_TECHNION_SURFBOARD.h>

uint8_t SDCardChipSelectPin = 5;
const int serialBaudRate = 115200;

// Create instances
WifiHandler* wifi;
Logger* logger;
SDCardHandler* sdCardHandler; 
Uploader* uploader;

// Replace with your Wi-Fi credentials
const char* ssid = "Loay's phone";
const char* password = "test123456";

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
  String macId = "12345";
 // uploader = new Uploader(sdCardHandler, macId);
  //uploader->begin();



}

void loop() {
  if(!wifi->isConnected()){
    wifi->reconnect();
  }else{
    // wifi connected
    logger->info(wifi->getIP().c_str());
    delay(500);
  }
  //uploader->update();
 // delay(10); // yield to FreeRTOS
}
