#include <IOT_TECHNION_SURFBOARD.h>

uint8_t SDCardChipSelectPin = 5;
const int serialBaudRate = 115200;

Logger* logger;
SDCardHandler* sdCardHandler;
Sampler* sampler;

const long samplingIntervalMillis = 5000;
int startTimeMillis;
bool isSampling;


void setup() {
  logger = Logger::getInstance();
  logger->init(serialBaudRate);
  logger->setLogLevel(LogLevel::DEBUG);
  sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
  sdCardHandler->init();

  //IMU_BNO080* IMU0 = new IMU_BNO080(logger,sdCardHandler, 100);
  //Force_HX711* force1 = new Force_HX711(logger,sdCardHandler, 300, 12, 13);
  Mock_HX711* mock2 = new Mock_HX711(logger,sdCardHandler, 30);

  sampler = new Sampler(logger,sdCardHandler, nullptr);
  sampler->init();
  //IMU0->init();
  //force1->init();
  mock2->init();
 // sampler->addSensor(IMU0);
 // sampler->addSensor(force1);
  sampler->addSensor(mock2);

  startTimeMillis = millis();
  sampler->startSampling(rand());
  isSampling = true;
}

void loop() {
  if(millis() - startTimeMillis < samplingIntervalMillis){
    sampler->writeSensorsData();
    delay(3);
  }else if(isSampling){
    sampler->stopSampling();
    isSampling=false;
  }else{
    // simulation stopped
    delay(1000);
  }
}



