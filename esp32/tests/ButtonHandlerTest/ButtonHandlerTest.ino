#include <IOT_TECHNION_SURFBOARD.h>

ButtonHandler* buttonHandler = nullptr;
Logger* logger = nullptr;

int buttonPin = 4;


void setup() {
    logger = Logger::getInstance();
    logger->init(115200);
    logger->setLogLevel(LogLevel::DEBUG);
    buttonHandler = new ButtonHandler(logger, buttonPin);
    buttonHandler->init();
}

void loop() {
    ButtonPressType press = buttonHandler->getLastPressType();
    if(press == ButtonPressType::LONG_PRESS){
        logger->info("Long press detected!");
    }else if(press == ButtonPressType::SOFT_PRESS){
        logger->info("soft press detected!");
    }
    delay(10);
}
