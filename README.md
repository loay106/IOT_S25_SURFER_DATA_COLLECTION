# IOT Winter 2024/2025 Surfer Data Collector by:
1. Loay Khateeb
2. Shada Hijazi
3. Mousa Abu Alhija

# Details about the project
Surfer Data Collection, a project under the ICST Lab at the Technion – Israel Institute of Technology, captures acceleration force data using multiple sensors strategically placed on the surfer's board. This innovative system provides deep insights into athlete movements and maneuvers, enhancing training and performance analysis with precise data collection.
Our project seamlessly manages wireless connectivity between sensors, ensuring efficient data transmission to the cloud for processing and analysis.

The project strives to offer an *efficient* and *robust* technological solution for the surfer! It consists of a single main unit and multiple sampling units (up to 20) that can read and write data from a variety of sensors, to better and enhance the surfer's experience and performance!

The project also utilizes the use of esp32's interrupts to maximize and offer a stable performance of the hardware.

The device can handle sensors with extremely high sampling rates, with high-frequency polling of approximately 1380 Hz.
Although the device offers this functionality, it's highly recommended to match the delay time with the highest sensor rate to save on CPU usage and extend the device's lifetime.

The device is also compatible with Firebase. Once the sampling session is complete, the data is persisted and is available on the cloud.

This project is the version 2.0 of the [previous](https://github.com/Moamena2/IOT---Surfboard-Data-Recorder.git) project.


# Folder description:
1. SurfboardMainUnit:
   
   This folder is used to program the Surfboard main unit which consists of the controller and a sampler.
   The device should only include one control unit.
   The control unit's properties, descriptions, and features are below.
   
2. SurfboardSamplingUnit:
   
   This folder is used to program the Surfboard sampling unit(s).
   The device is modular and can support multiple sampling units.
   The sampling unit's properties, descriptions, and features are below.

3. libraries/IOT_TECHNION_SURFBOARD:

   This folder includes a library that is used in SurfboardSamplingUnit and SurfboardMainUnit.
   Make sure to move this library to the Arduino libraries folder locally!
   
5. FireStore Files:

   This folder contains files integrated with Firestore to support file uploading, merging, and exporting.
   index.js includes the endpoints used to connect with Firebase and Cloud Storage.
   index.html is the user interface page hosted on Firestore.

6. tests:

   This folder includes tests for different parts of our project.
   
8. unit.config:

   This file includes WIFI details and sensor parameters that need to be provided.
   
# Arduino/ESP libraries used for the project:
   1. RTClib (by Adafruit) - version 2.1.4
   2. HX711 (by Rob Tillaart) - version 0.5.2
   3. SparkFun BNO080 Cortex Based IMU (by SparkFun) - version 1.1.12

# Installation:
1. Wire the necessary hardware as seen in the example diagram below and assemble your main and sampling units boxes
2. Move the libraries/IOT_TECHNION_SURFBOARD to your Arduino libraries folder (Documents\Arduino\libraries folder in Windows)
3. Add your parameters to unit.config file and move it to the top level in your SD card for each unit
4. Open SurfboardMainUnit/SurfboardMainUnit.ino file in your Arduino IDE, choose your wiring pins, and compile the code into your main unit
5. For every sampling unit desired, open SurfboardSamplingUnit/SurfboardSamplingUnit.ino in your Arduino IDE, update your sensors' data pins in the file and compile it
6. Setup your Firebase account, endpoints, storage, and host the index.html file
7. [RECOMMENDED] Limit the delay period in your unit's loop function to match the highest sensor's sampling rate
8. [RECOMMENDED] Press the button (Soft press) in your main unit and validate that the sampling process works.
9. [RECOMMENDED] Press the button (Long press) in your main unit and validate that the file uploading process works.
10. [RECOMMENDED] Write and keep track of your esp32's MAC address as it identifies the unit.

## Notes:
1. Sensors receive their IDs in the order they were added to the unit, starting from 0.
2. The sensor's parameters can be set in the unit.config file, in the same order they were inserted in
3. A Wi-Fi connection is required in the setup stage
4. To configure the calibration factor, edit the first value in the SENSORS_PARAMS list located in the unit.config file. For detailed instructions on how to calculate the calibration factor, please refer to the following link: [ESP32 with Load Cell and HX711 Amplifier (Digital Scale)](https://randomnerdtutorials.com/esp32-load-cell-hx711/) (Calibrating the Scale section). Please ensure that you adjust the pin numbers to match those specified in the wiring diagram.


# Hardware:
      
* The sampling unit's required hardware:
   1. Esp32 X1
   5. SD card X1

   Add sensors to this unit as desired

* The main unit's required hardware:
   1. Esp32 X1
   2. RTC X1
   3. Push Button X1
   4. RGB led light X1
   5. SD card X1
 
   You can add sensors to this unit as well
  
# wiring Diagram Example:
Example Sampling Unit Wiring Diagram:

<img src="https://github.com/user-attachments/assets/a86154c1-af3f-4eb6-b046-5d46ad49b0bc" alt="Image" width="600">

Example Main Unit Wiring Diagram:

<img src="https://github.com/user-attachments/assets/d2808a2c-2120-4290-80fc-dc2f74cc72e5" alt="Image" width="600">

# Known issues:
1. System crash in the upload process if Wifi is not available
2. File duplication if the upload process is interrupted before completion
3. ESP now protocol doesn't work correctly if wifi is connected with auto channel selection

# Project Poster:
![IOT_POSTER_page-0001](https://github.com/user-attachments/assets/ae712cdf-6002-4a4c-ab74-1cca1241ef37)


This project is part of ICST - The Interdisciplinary Center for Smart Technologies, Taub Faculty of Computer Science, Technion https://icst.cs.technion.ac.il/

   
