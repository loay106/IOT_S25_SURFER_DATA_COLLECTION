// esp32_loop_code.ino
#include <IOT_TECHNION_SURFBOARD.h>

// Create instances
WifiHandler wifi;
Logger* logger = Logger::getInstance();
SDCardHandler sdHandler;  // Assumed to handle SD initialization internally
Uploader* uploader;

// Replace with your Wi-Fi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Helper to retrieve MAC address without colons for unique hostname
String getMacIdentifier() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[13];
  snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void setup() {
  logger->init(SERIAL_BAUD_RATE);
  logger->setLogLevel(LogLevel::DEBUG);
  logger->info("Booting ESP32 test device");

  // Connect to Wi-Fi
  wifi.startConnection(ssid, password);

  // Wait until connected (non-blocking handled by update loop below)
  while (!wifi.isConnected()) {
    wifi.update();
    delay(100);
  }

  // Initialize SD Card (assumes internal logic is present)
  if (!sdHandler.begin()) {
    logger->error("Failed to initialize SD card");
    return;
  }

  // Start uploader with MAC as identifier
  String macId = getMacIdentifier();
  uploader = new Uploader(sdHandler, macId);
  uploader->begin();
}

void loop() {
  wifi.update();
  uploader->update();
  delay(10); // yield to FreeRTOS
}
