#include "SurfboardMainUnit.h"
#include <HTTPClient.h>

SurfboardMainUnit::SurfboardMainUnit(ControlUnitSyncManager* syncManager, RTCTimeHandler* timeHandler, RGBStatusHandler* statusLighthandler, ButtonHandler* buttonHandler, Logger* logger, Sampler* sampler, SDCardHandler* sdCardHandler, WirelessHandler* wirelessHandler, DataCollectorServer* server) {
  this->logger = logger;
  this->syncManager = syncManager;
  this->timeHandler = timeHandler;
  this->statusLighthandler = statusLighthandler;
  this->buttonHandler = buttonHandler;
  this->logger = logger;
  this->sampler = sampler;
  this->sdCardHandler = sdCardHandler;
  this->wirelessHandler = wirelessHandler;
  this->server = server;
  status = SystemStatus::SYSTEM_STARTING;
  currentSamplingSession = 0;
}

void SurfboardMainUnit::init(vector<uint8_t*> samplingUnitsMacAddresses) {
  for (int i = 0; i < samplingUnitsMacAddresses.size(); i++) {
    SamplingUnitRep samplingUnit;
    memcpy(samplingUnit.mac, samplingUnitsAdresses[i], 6);
    samplingUnit.status = SamplerStatus::UNIT_STAND_BY;
    samplingUnit.lastCommandSentMillis = 0;
    samplingUnit.lastStatusUpdateMillis = 0;
    String macString = macToString(samplingUnit.mac);
    samplingUnit.mDNSHostname = getHostname(macString, false) + ".local";
    samplingUnits[macString] = samplingUnit;
  }
  updateStatus(SystemStatus::SYSTEM_STAND_BY);
}

void SurfboardMainUnit::updateStatus(SystemStatus newStatus) {
  if (status == newStatus) {
    return;
  }
  status = newStatus;
  switch (status) {
    case SystemStatus::SYSTEM_STARTING:
      break;
    case SystemStatus::SYSTEM_STAND_BY:
      statusLighthandler->updateColors(RGBColors::BLUE, RGBColors::BLUE);
      break;
    case SystemStatus::SYSTEM_STAND_BY_PARTIAL_ERROR:
      statusLighthandler->updateColors(RGBColors::RED, RGBColors::BLUE);
      break;
    case SystemStatus::SYSTEM_SAMPLING:
      statusLighthandler->updateColors(RGBColors::GREEN, RGBColors::NO_COLOR);
      break;
    case SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR:
      statusLighthandler->updateColors(RGBColors::GREEN, RGBColors::RED);
      break;
    case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD:
      statusLighthandler->updateColors(RGBColors::CYAN, RGBColors::NO_COLOR);
      break;
    case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING:
      statusLighthandler->updateColors(RGBColors::CYAN, RGBColors::BLUE);
      break;
    case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_PARTIAL_ERROR:
      statusLighthandler->updateColors(RGBColors::CYAN, RGBColors::RED);
      break;
    case SystemStatus::SYSTEM_ERROR:
      statusLighthandler->updateColors(RGBColors::RED, RGBColors::RED);
      break;
    default:
      statusLighthandler->updateColors(RGBColors::NO_COLOR, RGBColors::NO_COLOR);
      break;
  }
}

void SurfboardMainUnit::addSensor(SensorBase* sensor) {
  sampler->addSensor(sensor);
}

void SurfboardMainUnit::startSampling() {
  if(wirelessHandler->getCurrentMode() != WirelessHandler::MODE::ESP_NOW || !wirelessHandler->isConnected()){
    wirelessHandler->switchToESPNow();
    updateStatus(SystemStatus::SYSTEM_STAND_BY_PARTIAL_ERROR);
  }

  currentSamplingSession = timeHandler->getCurrentTimestamp();
  std::map<String, String> samplingParams;
  samplingParams["TIMESTAMP"] = String(currentSamplingSession);
  try {
    syncManager->broadcastESPNowCommand(ControlUnitCommand::START_SAMPLING, samplingParams);
  } catch (ESPNowSyncError& error) {
    logger->error(F("Failed to send command to sampling units! Try again!"));
    updateStatus(SystemStatus::SYSTEM_STAND_BY_PARTIAL_ERROR);
    return;
  }

  unsigned long current = millis();
  std::map<String, SamplingUnitRep>::iterator it;
  for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
    it->second.lastCommandSentMillis = current;
  }

  sampler->startSampling(currentSamplingSession);
  updateStatus(SystemStatus::SYSTEM_SAMPLING);
  // logger->info("Sampling started...");
}

void SurfboardMainUnit::stopSampling() {
  if(wirelessHandler->getCurrentMode() != WirelessHandler::MODE::ESP_NOW || !wirelessHandler->isConnected()){
      wirelessHandler->switchToESPNow();
      delay(300);
  }

  try {
    std::map<String, String> params;
    syncManager->broadcastESPNowCommand(ControlUnitCommand::STOP_SAMPLING, params);
  } catch (ESPNowSyncError& error) {
    logger->error(F("Failed to send command STOP_SAMPLING to sampling units!"));
    sampler->stopSampling();
    updateStatus(SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR);
    return;
  }

  unsigned long current = millis();
  std::map<String, SamplingUnitRep>::iterator it;
  for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
    it->second.lastCommandSentMillis = current;
  }

  sampler->stopSampling();
  updateStatus(SystemStatus::SYSTEM_STAND_BY);
}


void SurfboardMainUnit::startSampleFilesUpload() {
  updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STARTING);
  logger->info(F("File upload starting..."));

  if(wirelessHandler->getCurrentMode() != WirelessHandler::MODE::ESP_NOW || !wirelessHandler->isConnected()){
      wirelessHandler->switchToESPNow();
      delay(300);
  }

  std::map<String, String> params;
  params["WIFI_SSID"] = WIFI_SSID;
  params["WIFI_PASSWORD"] = WIFI_PASSWORD;

  for (int i = 0; i < 3; i++) {
    try {
      syncManager->broadcastESPNowCommand(ControlUnitCommand::START_SAMPLE_FILES_UPLOAD, params);
    } catch (ESPNowSyncError& error) {
      logger->error(F("Failed to send command to sampling units! Try again!"));
      updateStatus(SystemStatus::SYSTEM_STAND_BY_PARTIAL_ERROR);
      return;
    }
  }

  unsigned long current = millis();
  std::map<String, SamplingUnitRep>::iterator it;
  for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
    it->second.lastCommandSentMillis = current;
  }

  delay(200);  // give some time for units to respond

  server->begin();
  logger->info(F("File upload starting..."));
  updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD);

  if(wirelessHandler->getCurrentMode() != WirelessHandler::MODE::WIFI){
      wirelessHandler->switchToWifi();
  }
}

void SurfboardMainUnit::stopSampleFilesUpload() {
  updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING);
  logger->info(F("Stopping file upload..."));
}


SystemStatus SurfboardMainUnit::getStatus() {
  SystemStatus res = status;
  return res;
}

void SurfboardMainUnit::handleButtonPress() {
  ButtonPressType press = buttonHandler->getLastPressType();
  if (press != ButtonPressType::NO_PRESS) {
    switch (status) {
      case SystemStatus::SYSTEM_SAMPLING:
      case SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR:
        logger->debug(F("Soft or long press detected while sampling"));
        stopSampling();
        break;

      case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD:
        logger->debug(F("Soft or long press detected while file uploading"));
        stopSampleFilesUpload();
        break;

      case SystemStatus::SYSTEM_STAND_BY:
      case SystemStatus::SYSTEM_STAND_BY_PARTIAL_ERROR:
        if (press == ButtonPressType::SOFT_PRESS) {
          logger->debug(F("Soft press detected while on standby"));
          startSampling();
        } else {
          logger->debug(F("Long press detected while on standby"));
          startSampleFilesUpload();
        }
        break;
      case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING:
      case SystemStatus::SYSTEM_ERROR:
          logger->debug("Press was ignored while on file upload stopping or general error");
    }
    delay(100);  // give some time for status messages
  }
}

void SurfboardMainUnit::readStatusUpdateMessages() {
  while (syncManager->hasStatusUpdateMessages()) {
    StatusUpdateMessage statusMessage = ControlUnitSyncManager::popStatusUpdateMessage();
    String unitID = macToString(statusMessage.from);

    try {
      SamplingUnitRep& samplingUnit = samplingUnits.at(unitID);
      samplingUnit.lastStatusUpdateMillis = millis();

      switch (statusMessage.status) {
        case SamplingUnitStatusMessage::STAND_BY:
          samplingUnit.status = SamplerStatus::UNIT_STAND_BY;
          logger->info("Unit " + unitID + " reported STAND BY");
          break;

        case SamplingUnitStatusMessage::SAMPLING:
          samplingUnit.status = SamplerStatus::UNIT_SAMPLING;
          logger->info("Unit " + unitID + " reported SAMPLING");
          break;

        case SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD:
          samplingUnit.status = SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD;
          logger->info("Unit " + unitID + " reported SAMPLING FILE UPLOADING");
          break;

        case SamplingUnitStatusMessage::ERROR:
          samplingUnit.status = SamplerStatus::UNIT_ERROR;
          logger->info("Unit " + unitID + " reported ERROR");
          break;
      }
    } catch (const std::out_of_range& ex) {
      logger->error("Status update message received from unknown unit " + unitID);
    }
  }
}


void SurfboardMainUnit::sendCommand(SamplingUnitRep& unit, ControlUnitCommand command, std::map<String, String> commandParams) {
  unsigned long current = millis();
  if ((current - unit.lastCommandSentMillis) < COMMAND_SEND_MIN_INTERVAL_MILLIS) {
    return;
  }

  if(wirelessHandler->getCurrentMode() == WirelessHandler::MODE::ESP_NOW || wirelessHandler->isConnected()){
      // send command via esp now
      try{
          syncManager->sendESPNowCommand(ControlUnitCommand::START_SAMPLING, commandParams, unit.mac);
          unit.lastCommandSentMillis = millis();
      }catch (ESPNowSyncError& error) {
          unit.status = SamplerStatus::UNIT_ERROR;
      }
  }else if(wirelessHandler->getCurrentMode() == WirelessHandler::MODE::WIFI || wirelessHandler->isConnected()){
      // send command via wifi
  }
}

unsigned long LAST_WIRELESS_CONNECTION_SWITCH = 0;
int WIRELESS_CONNECTION_SWICH_INTERVAL_MILLIS = 120000;
void SurfboardMainUnit::loopFileUpload() {
  unsigned long current = millis();
  WirelessHandler::MODE currentConnectionMode = wirelessHandler->getCurrentMode();
  if((current - LAST_WIRELESS_CONNECTION_SWITCH) >= WIRELESS_CONNECTION_SWICH_INTERVAL_MILLIS){
      logger->info("Switching connection mode, currently: " + mode_to_string(currentConnectionMode));
      if(currentConnectionMode == WirelessHandler::MODE::WIFI){
          wirelessHandler->switchToESPNow();
      }else{
          wirelessHandler->switchToWifi();
      }
      LAST_WIRELESS_CONNECTION_SWITCH = millis();
      return;
  }


  std::map<String, SamplingUnitRep>::iterator it;
  int unitsStoppedNum = 0;
  for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
    if(getStatus() == SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING && it->second.status == SamplerStatus::UNIT_STAND_BY){
      unitsStoppedNum++;
    }else{
        // status == SYSTEM_SAMPLE_FILE_UPLOAD or SYSTEM_SAMPLE_FILE_UPLOAD_PARTIAL_ERROR
    }




    if ((current - it->second.lastStatusUpdateMillis) >= STATUS_REPORT_DELAY_MILLIS) {
      try{
          syncManager->pingServerWifi(it->first);
      }catch(...){

      }
      
      bool result = pingSamplingUnitDataServer(it->second.mDNSHostname);
      if (result) {
        StatusUpdateMessage statusMessage;
        memcpy(statusMessage.from, it->second.mac, 6);
        statusMessage.status = SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD;
        syncManager->addStatusUpdateMessage(statusMessage);
      }
    }
  }
}

void SurfboardMainUnit::loopSampling() {
  if (!sampler->isSampling()) {
    sampler->startSampling(currentSamplingSession);
  } else {
    sampler->writeSensorsData();
  }

  std::map<String, SamplingUnitRep>::iterator it;
  for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
    SamplerStatus unitStatus = it->second.status;
    if (unitStatus == SamplerStatus::UNIT_ERROR) {
      updateStatus(SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR);
    }
    if (unitStatus != SamplerStatus::UNIT_SAMPLING) {
      std::map<String, String> commandParams;
      commandParams["TIMESTAMP"] = String(currentSamplingSession);
      sendCommand(it->second, ControlUnitCommand::START_SAMPLING, commandParams);
    }
  }
}

void SurfboardMainUnit::loopStandby() {
  if (sampler->isSampling()) {
    sampler->stopSampling();
  }

  std::map<String, SamplingUnitRep>::iterator it;
  for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
    SamplerStatus unitStatus = it->second.status;
    if (unitStatus == SamplerStatus::UNIT_SAMPLING) {
      std::map<String, String> commandParams;
      sendCommand(it->second, ControlUnitCommand::STOP_SAMPLING, commandParams);
    }
  }
}

void SurfboardMainUnit::loopDiscoverDisconnected() {
  unsigned long current = millis();
  std::map<String, SamplingUnitRep>::iterator it;
  for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
    if ((current - it->second.lastStatusUpdateMillis) >= MAX_STATUS_UPDATE_DELAY && it->second.status != SamplerStatus::UNIT_ERROR) {
      logger->info(String("Unit ") + it->first + String(" is disconnected!"));
      it->second.status = SamplerStatus::UNIT_ERROR;
    }
  }
}

void SurfboardMainUnit::loopComponents{
  try{
      wirelessHandler->loop();
  }catch(...){
    logger->error("Wireless handler error..");
  } 
}
