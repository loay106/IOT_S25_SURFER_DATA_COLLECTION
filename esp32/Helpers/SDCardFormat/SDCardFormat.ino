#include <SPI.h>
#include <SD.h>

const uint8_t SD_CS_PIN = 5;  // Change if needed for your board

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Initializing SD card...");

  // Initialize SPI and SD
  SPI.begin(18, 19, 23, SD_CS_PIN); // Default VSPI pins for ESP32
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }

  Serial.println("SD card initialized.");
  Serial.println("Formatting: deleting all files and folders...");

  formatSD(SD, "/");

  Serial.println("Formatting complete.");
}

void loop() {
  // Nothing here
}

// Recursively deletes files and directories from given path
void formatSD(fs::FS &fs, const char * path) {
  File root = fs.open(path);
  if (!root) {
    Serial.println("Failed to open directory.");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory.");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String filePath = String(path) + "/" + file.name();
    if (file.isDirectory()) {
      formatSD(fs, filePath.c_str());
      fs.rmdir(filePath.c_str());
      Serial.printf("Deleted folder: %s\n", filePath.c_str());
    } else {
      fs.remove(filePath.c_str());
      Serial.printf("Deleted file: %s\n", filePath.c_str());
    }
    file = root.openNextFile();
  }
}
