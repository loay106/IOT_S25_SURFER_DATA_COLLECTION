# IOT Spring 2025 Surfer Data Collector by Loay Khateeb

## Details about the project
This project is version 3.0 of the [previous](https://github.com/loay106/IOT_W25_SURFER_DATA_COLLECTION) project.


## Changelog:
   1. Fix issue with system crash when wifi is not available.
   2. Fix issue with duplicate files in the upload process.
   3. Fix issue with esp-now protocl connecting to a channel different than wifi channel.
   4. Add file validation (MD5) in the upload process to ensure file integrity during the process.
   5. Chnage the upload process so units can now create a local server and the user can interact, download sampling files and configure the system through a user friendly python client.
   6. Add helper ino files for continuous development .

## Installation:
   1. Wire the main and sampling units with the desired sensors on them, as described in the previous project.
   2. Add main.config file to the top level of the SD card in the main unit (the script in /esp32/Helpers/SDCardMainUnitConfigWriter can be used for ease of use).
   3. Add unit.config file to the top level of the SD card in each of the samplings unit (the script in /esp32/Helpers/SDCardSamplingUnitConfigWriter can be used for ease of use).
   4. Compile the code in /esp32/SurfboardMainUnit to the main unit.
   5. Compile the code in /esp32/SurfboardSamplingUnit to each one of the sampling units.

## Statuses
   The main unit triggers different RGB colors to indicate the current status of the system:

   1. Blue: System standby
   2. Green: System sampling
   3. Cyan: System file upload

   These statuses are controlled by the main unit, which communicates with the other units to keep the entire system synchronized.

   A combination of any one of the colors above with red indicates a failure in the process.

   All failures/errors are recoverable by the system, except for hardware failures and wrong inputs (Wrong or absent wifi credentials for instance).

## Usage
   1. Soft press on the main unit's button starts the sampling process when the system is in standby.
   2. Long press on the main unit's button starts the file uploading process when the system is in standby.
   3. Soft or long press on the main unit's button cancels the above mode and reverts the system back to standby mode.

## File Upload Mode
   When the system enters the file upload mode, the main unit connects to the provided wifi credentials in the main.config file, along with all the sampling units.

   In this mode, each unit hosts a local server and exposes certain endpoints which allow the process to be completed.

   The /client folder includes a python program, with a user friendly UI, for discovering these local servers in the network and interacting with them.

   
## Arduino/ESP libraries used for the project:
   1. RTClib (by Adafruit) - version 2.1.4
   2. HX711 (by Rob Tillaart) - version 0.5.2
   3. SparkFun BNO080 Cortex Based IMU (by SparkFun) - version 1.1.12
   4. Async TCP (by ESP32Async) - version 3.3.8
   5. ESP Async WebServer (by ESP32Async) - version 3.7.6


   
