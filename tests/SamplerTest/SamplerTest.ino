#include <IOT_TECHNION_SURFBOARD.h>

uint8_t SDCardChipSelectPin = 5;
const int serialBaudRate = 115200;

Logger* logger;
SDCardHandler* sdCardHandler;
Sampler* sampler;

const long SAMPLING_PERIOD_MILLIES = 1800000;
int startTimeMillis;

void setup() {
  logger = Logger::getInstance();
  logger->init(serialBaudRate);
  logger->setLogLevel(LogLevel::DEBUG);
  sdCardHandler = new SDCardHandler(SDCardChipSelectPin, logger);
  sdCardHandler->init();

  //IMU_BNO080* IMU0 = new IMU_BNO080(logger,sdCardHandler, 100);
  //Force_HX711* force1 = new Force_HX711(logger,sdCardHandler, 300, 12, 13);
  Mock_HX711* mock1 = new Mock_HX711(logger,sdCardHandler, 1500);
  Mock_HX711* mock2 = new Mock_HX711(logger,sdCardHandler, 1500);
  Mock_HX711* mock3 = new Mock_HX711(logger,sdCardHandler, 1500);
  Mock_HX711* mock4 = new Mock_HX711(logger,sdCardHandler, 1500);

  sampler = new Sampler(logger,sdCardHandler);
  sampler->init();
  //IMU0->init();
  //force1->init();
  mock1->init();
  mock2->init();
  mock3->init();
  mock4->init();
 // sampler->addSensor(IMU0);
 // sampler->addSensor(force1);
  sampler->addSensor(mock1);
  sampler->addSensor(mock2);
  sampler->addSensor(mock3);
  sampler->addSensor(mock4);

  startTimeMillis = millis();
  sampler->startSampling(1744823385);
}

void loop() {
  if(millis() - startTimeMillis < SAMPLING_PERIOD_MILLIES){
    sampler->writeSensorsData();
  }else if(sampler->isSampling()){
    sampler->stopSampling();
  }else{
    // simulation stopped
    delay(1000);
  }
}



