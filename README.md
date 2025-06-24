# IOT Spring 2025 Surfer Data Collector by Loay Khateeb

## Details about the project
This project is version 3.0 of the [previous](https://github.com/loay106/IOT_W25_SURFER_DATA_COLLECTION) project.


## Changelog:
   1. Fix issue with system crash when wifi is not available
   2. Fix issue with duplicate files in the upload process
   3. Fix issue with esp-now protocl connecting to a channel different than wifi channel
   4. Add file validation (MD5) in the upload process to ensure file integrity during the process
   5. Chnage the upload process so units can now create a local server and the user can interact, download sampling files and configure the system through a user friendly python client
   6. Add helper ino files for continuous development 
   
## Arduino/ESP libraries used for the project:
   1. RTClib (by Adafruit) - version 2.1.4
   2. HX711 (by Rob Tillaart) - version 0.5.2
   3. SparkFun BNO080 Cortex Based IMU (by SparkFun) - version 1.1.12
   4. AsyncTCP (ESP32Async)
   5. ESPAsyncWebServer (ESP32Async)


   
