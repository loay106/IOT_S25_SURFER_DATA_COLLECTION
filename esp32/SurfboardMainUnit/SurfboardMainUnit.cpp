#include "SurfboardMainUnit.h"

SurfboardMainUnit::SurfboardMainUnit(ControlUnitSyncManager* syncManager, RTCTimeHandler* timeHandler, RGBStatusHandler* statusLighthandler, ButtonHandler* buttonHandler, Logger* logger, Sampler* sampler, SDCardHandler* sdCardHandler, WirelessHandler* wirelessHandler, const String& _wifiSSID, const String& _wifiPassword, DataCollectorServer* server) {
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
  this->WIFI_SSID = _wifiSSID;
  this->WIFI_PASSWORD = _wifiPassword;
  currentSamplingSession = 0;
}

void SurfboardMainUnit::init(vector<uint8_t*> samplingUnitsMacAddresses) {
  for (int i = 0; i < samplingUnitsMacAddresses.size(); i++) {
    SamplingUnitRep samplingUnit;
    memcpy(samplingUnit.mac, samplingUnitsMacAddresses[i], 6);
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
    case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_WIFI_ERROR:
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
    if(!samplingUnits.empty()){
        syncManager->broadcastESPNowCommand(ControlUnitCommand::START_SAMPLING, samplingParams);
    }
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
      if(!samplingUnits.empty()){
          syncManager->broadcastESPNowCommand(ControlUnitCommand::STOP_SAMPLING, params);
      } 
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
  if(wirelessHandler->getCurrentMode() != WirelessHandler::MODE::ESP_NOW || !wirelessHandler->isConnected()){
      wirelessHandler->switchToESPNow();
      updateStatus(SystemStatus::SYSTEM_STAND_BY_PARTIAL_ERROR);
      logger->info(F("Failed to start file upload because esp now is not connected. Try again later..."));
      return;
  }

  std::map<String, String> params;
  params["WIFI_SSID"] = WIFI_SSID;
  params["WIFI_PASSWORD"] = WIFI_PASSWORD;
  for (int i = 0; i < 3; i++) {
    try {
      if(!samplingUnits.empty()){
          syncManager->broadcastESPNowCommand(ControlUnitCommand::START_SAMPLE_FILES_UPLOAD, params);
      }
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
  server->begin();
  logger->info(String("File upload started..."));
  updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD);
}

void SurfboardMainUnit::stopSampleFilesUpload() {
  updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING);
  logger->info(String("Stopping file upload..."));
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
      case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING:
      case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_WIFI_ERROR:
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
      samplingUnit.status = statusMessage.status;
      logger->info("Unit " + unitID + " reported " + sampler_status_to_string(statusMessage.status));
    } catch (const std::out_of_range& ex) {
      logger->error("Status update message received from unknown unit " + unitID);
    }
  }
}


void SurfboardMainUnit::sendESPNowCommand(SamplingUnitRep& unit, ControlUnitCommand command, std::map<String, String> commandParams) {
  unsigned long current = millis();
  if ((current - unit.lastCommandSentMillis) < COMMAND_SEND_MIN_INTERVAL_MILLIS) {
    return;
  }

  if(wirelessHandler->getCurrentMode() == WirelessHandler::MODE::ESP_NOW && wirelessHandler->isConnected()){
      // send command via esp now
      try{
          syncManager->sendESPNowCommand(ControlUnitCommand::START_SAMPLING, commandParams, unit.mac);
          unit.lastCommandSentMillis = millis();
      }catch (ESPNowSyncError& error) {
          unit.status = SamplerStatus::UNIT_ERROR;
      }
  }
}

void SurfboardMainUnit::loopFileUpload(SystemStatus status) {
  unsigned long current = millis();
  WirelessHandler::MODE currentConnectionMode = wirelessHandler->getCurrentMode();

  // handle invalid wifi connection here
  if(status == SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_WIFI_ERROR){
    if(currentConnectionMode != WirelessHandler::MODE::WIFI){
        wirelessHandler->switchToWifi(WIFI_SSID, WIFI_PASSWORD);
    }else if(wirelessHandler->isConnected()){
        updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD);
    }
    return;

  } else if(status == SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD){
      if(currentConnectionMode == WirelessHandler::MODE::WIFI){
          // check if wifi is connected and/or available
          if(!wirelessHandler->isConnected() && (current - wirelessHandler->getCurrentModeStartTime()) >= WIFI_CONNECTION_RETRY_TIMEOUT_MILLIS){
              updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_WIFI_ERROR);
              return;
          }else{
            // wifi is connected
            int numUnitsNotInFileUploadMode = 0;

            std::map<String, SamplingUnitRep>::iterator it;
            for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
                // check if in file upload mode
                if(it->second.status != SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
                    numUnitsNotInFileUploadMode++;
                }
                // ping each unit to make sure its connected to wifi (serves as a reverse status report)
                try{
                    if ((current - it->second.lastStatusUpdateMillis) >= STATUS_REPORT_DELAY_MILLIS) {
                        syncManager->pingServerWifi(it->first);
                        StatusUpdateMessage statusMessage;
                        memcpy(statusMessage.from, it->second.mac, 6);
                        statusMessage.status = SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD;
                        syncManager->addStatusUpdateMessage(statusMessage);
                    }
                }catch(...){
                        StatusUpdateMessage statusMessage;
                        memcpy(statusMessage.from, it->second.mac, 6);
                        statusMessage.status = SamplerStatus::UNIT_ERROR;
                        syncManager->addStatusUpdateMessage(statusMessage);
                }
            }

            if(numUnitsNotInFileUploadMode != 0){
                wirelessHandler->switchToESPNow();
            }
          }
      } else if(currentConnectionMode == WirelessHandler::MODE::ESP_NOW){
        if((current - wirelessHandler->getCurrentModeStartTime()) >= FILE_UPLOAD_ESP_NOW_CONNECTION_LIMIT_MILLIS){
            wirelessHandler->switchToWifi(WIFI_SSID, WIFI_PASSWORD);
            return;
        }else if(wirelessHandler->isConnected()){
            std::map<String, SamplingUnitRep>::iterator it;
            std::map<String, String> commandParams;
            commandParams["WIFI_SSID"] = WIFI_SSID;
            commandParams["WIFI_PASSWORD"] = WIFI_PASSWORD;
            for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
              if(it->second.status != SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
                  sendESPNowCommand(it->second, ControlUnitCommand::START_SAMPLE_FILES_UPLOAD, commandParams);
              }
            }
        }
      }
    } else if(status == SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING){
      // handle status switch from FILE_UPLOAD to STAND_BY
      if((current - wirelessHandler->getCurrentModeStartTime()) >= FILE_UPLOAD_STOP_CONNECTION_SWITCH_INTERVAL){
          if(currentConnectionMode == WirelessHandler::MODE::WIFI){
              wirelessHandler->switchToESPNow();
          }else{
              wirelessHandler->switchToWifi(WIFI_SSID, WIFI_PASSWORD);
          }
          return;
      }
      std::map<String, SamplingUnitRep>::iterator it;
      int numUnitsInStandBy = 0;
      for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
          if(it->second.status != SamplerStatus::UNIT_STAND_BY){
              if(currentConnectionMode == WirelessHandler::MODE::WIFI){
                try{
                    syncManager->sendWifiStopFileUploadCommand(it->first);
                    StatusUpdateMessage statusMessage;
                    memcpy(statusMessage.from, it->second.mac, 6);
                    statusMessage.status = SamplerStatus::UNIT_STAND_BY;
                    syncManager->addStatusUpdateMessage(statusMessage);
                }catch(...){
                    StatusUpdateMessage statusMessage;
                    memcpy(statusMessage.from, it->second.mac, 6);
                    statusMessage.status = SamplerStatus::UNIT_ERROR;
                    syncManager->addStatusUpdateMessage(statusMessage);
                }
              } else if(currentConnectionMode == WirelessHandler::MODE::ESP_NOW){
                  std::map<String, String> commandParams;
                  sendESPNowCommand(it->second, ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD,commandParams);
              }
          }else{
              numUnitsInStandBy++;
          }
      }

      if(numUnitsInStandBy == samplingUnits.size()){
          // all units are on standby
          wirelessHandler->switchToESPNow();
          updateStatus(SystemStatus::SYSTEM_STAND_BY);
      }
    }
}

void SurfboardMainUnit::loopSampling() {
  if (!sampler->isSampling()) {
    sampler->startSampling(currentSamplingSession);
  } else {
    sampler->writeSensorsData();
  }

  // todo: handle case where esp is now on wifi mode
  // should send to esp units to stop, then send them  to start sampling
  // or just make sure we never get here in wifi mode...

  std::map<String, SamplingUnitRep>::iterator it;
  for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
    SamplerStatus unitStatus = it->second.status;
    if (unitStatus == SamplerStatus::UNIT_ERROR) {
      updateStatus(SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR);
    }
    if (unitStatus != SamplerStatus::UNIT_SAMPLING) {
      std::map<String, String> commandParams;
      commandParams["TIMESTAMP"] = String(currentSamplingSession);
      sendESPNowCommand(it->second, ControlUnitCommand::START_SAMPLING, commandParams);
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
      sendESPNowCommand(it->second, ControlUnitCommand::STOP_SAMPLING, commandParams);
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

void SurfboardMainUnit::loopComponents(){
  try{
      wirelessHandler->loop();
  }catch(...){
    logger->error("Wireless handler error..");
  } 
}
