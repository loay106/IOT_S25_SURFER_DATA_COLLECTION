#include "SurfboardMainUnit.h"

SurfboardMainUnit::SurfboardMainUnit(ControlUnitSyncManager *syncManager, RTCTimeHandler* timeHandler, RGBStatusHandler* statusLighthandler, ButtonHandler *buttonHandler, Logger *logger, Sampler* sampler, SDCardHandler* sdCardHandler){
    this->logger = logger;
    this->syncManager = syncManager;
    this->timeHandler = timeHandler;
    this->statusLighthandler = statusLighthandler;
    this->buttonHandler = buttonHandler;
    this->logger = logger;
    this->sampler = sampler;
    this->sdCardHandler = sdCardHandler;
    this->uploadStartTime = 0;
    status = SystemStatus::SYSTEM_STARTING;
    currentSamplingSession = 0;
}

void SurfboardMainUnit::init(uint8_t samplingUnitsAdresses[][6], int samplingUnitsNum) {
    // add sampling units
    for(int i=0;i<samplingUnitsNum;i++){
        SamplingUnitRep samplingUnit;
        memcpy(samplingUnit.mac, samplingUnitsAdresses[i], 6);
        samplingUnit.status = SamplerStatus::UNIT_STAND_BY;
        samplingUnit.hasFilesToUpload = false;
        samplingUnit.lastCommandSentMillis = 0;
        samplingUnit.lastStatusUpdateMillis = 0;
        string macString = macToString(samplingUnit.mac);
        samplingUnits[macString] = samplingUnit;
    }
    syncManager->connect();
    updateStatus(SystemStatus::SYSTEM_STAND_BY);
}

void SurfboardMainUnit::updateStatus(SystemStatus newStatus){
    if(status == newStatus){
        return;
    }
    status = newStatus;
    switch (status){
        case SystemStatus::SYSTEM_STARTING:
            break;
        case SystemStatus::SYSTEM_STAND_BY:
            statusLighthandler->updateColors(RGBColors::BLUE, RGBColors::BLUE);
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
    currentSamplingSession = timeHandler->getCurrentTimestamp();
    std::map<string, string> samplingParams;
    samplingParams["TIMESTAMP"] = to_string(currentSamplingSession);
    try{
        syncManager->broadcastCommand(ControlUnitCommand::START_SAMPLING, samplingParams);
    }catch(ESPNowSyncError& error){
        logger->error("Failed to send command to sampling units! Try again!");
        return;
    }
    unsigned long current = millis();
    std::map<string, SamplingUnitRep>::iterator it;
    for (it=samplingUnits.begin(); it!=samplingUnits.end(); it++) {
      it->second.hasFilesToUpload = true;
      it->second.lastCommandSentMillis = current;
    }
    sampler->startSampling(currentSamplingSession);
    updateStatus(SystemStatus::SYSTEM_SAMPLING);
    //logger->info("Sampling started...");
}

void SurfboardMainUnit::stopSampling() {
    try{
        std::map<string, string> params;
        syncManager->broadcastCommand(ControlUnitCommand::STOP_SAMPLING, params);
    }catch(ESPNowSyncError& error){
        return;
    }
    unsigned long current = millis();
    std::map<string, SamplingUnitRep>::iterator it;
    for (it=samplingUnits.begin(); it!=samplingUnits.end(); it++) {
      it->second.lastCommandSentMillis = current;
    }
    sampler->stopSampling();
    updateStatus(SystemStatus::SYSTEM_STAND_BY);
}

void SurfboardMainUnit::startSampleFilesUpload() {
    try{
        std::map<string, string> params;
        syncManager->broadcastCommand(ControlUnitCommand::START_SAMPLE_FILES_UPLOAD, params);
    }catch(ESPNowSyncError& error){
        logger->error("Failed to send command to sampling units! Try again!");
        return;
    }

    logger->info("File upload started!");
    unsigned long current = millis();
    std::map<string, SamplingUnitRep>::iterator it;
    for (it=samplingUnits.begin(); it!=samplingUnits.end(); it++) {
      it->second.lastCommandSentMillis = current;
    }

    if(sampler->hasFilesToCloudUpload()){
      if(sampler->getStatus() != SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
          logger->debug("Uploading internal sampler data started");
          try{
              sampler->connect();
          }catch(...){
              updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_PARTIAL_ERROR);
              return;
          }
      }
    }
    uploadStartTime = 0;
    updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD);
}

void SurfboardMainUnit::stopSampleFilesUpload() {
    try{
        std::map<string, string> params;
        syncManager->broadcastCommand(ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD, params);
    }catch(ESPNowSyncError& error){
        logger->error("Failed to send command to sampling units! Try again!");
        return;
    }
    unsigned long current = millis();
    std::map<string, SamplingUnitRep>::iterator it;
    for (it=samplingUnits.begin(); it!=samplingUnits.end(); it++) {
      it->second.lastCommandSentMillis = current;
    }
    sampler->disconnect();
    //syncManager->disconnect();
    // syncManager->connect();
    updateStatus(SystemStatus::SYSTEM_STAND_BY);
    unsigned long elapsedTime = (current - uploadStartTime)/1000;
    logger->info("File upload stopped! Process ended in " + to_string(elapsedTime) + " seconds");
}

SystemStatus SurfboardMainUnit::getStatus(){
    SystemStatus res = status;
    return res;
}

void SurfboardMainUnit::handleButtonPress(){
    ButtonPressType press = buttonHandler->getLastPressType();
    if(press != ButtonPressType::NO_PRESS){
        switch(status){
            case SystemStatus::SYSTEM_SAMPLING:
            case SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR:
                logger->debug("Soft or long press detected while sampling");
                stopSampling();
                break;
            case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD:
            case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_PARTIAL_ERROR:
                logger->debug("Soft or long press detected while file uploading");
                stopSampleFilesUpload();
                break;
            case SystemStatus::SYSTEM_STAND_BY:
                if(press == ButtonPressType::SOFT_PRESS){
                    logger->debug("Soft press detected while on standby");
                    startSampling();
                }else{
                    // long press
                    logger->debug("Long press detected while on standby");
                    startSampleFilesUpload();
                }
                break;
        }
      delay(200); // give some time for status messages
    }
}

void SurfboardMainUnit::readStatusUpdateMessages(){
    while(syncManager->hasStatusUpdateMessages()){
        // todo: update in ControlUnitSyncManager and throw error in case is empty
        StatusUpdateMessage statusMessage = ControlUnitSyncManager::popStatusUpdateMessage();
        string unitID = macToString(statusMessage.from);
        try{
            SamplingUnitRep& samplingUnit = samplingUnits.at(unitID);
            samplingUnit.lastStatusUpdateMillis = millis();
            switch(statusMessage.status){
                case SamplingUnitStatusMessage::STAND_BY:
                  samplingUnit.status = SamplerStatus::UNIT_STAND_BY;
                  logger->info("Unit " + unitID + " reported STAND BY");
                  break;
                case SamplingUnitStatusMessage::SAMPLING:
                  logger->info("Unit " + unitID + " reported SAMPLING");
                  samplingUnit.status = SamplerStatus::UNIT_SAMPLING;
                  break;
                case SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD:
                  logger->info("Unit " + unitID + " reported SAMPLING FILE UPLOADING");
                  samplingUnit.status = SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD;
                  break;
                case SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD_COMPLETE:
                  logger->info("Unit " + unitID + " reported SAMPLING FILE UPLOAD COMPLETED!");
                  samplingUnit.hasFilesToUpload = false;
                  samplingUnit.status = SamplerStatus::UNIT_STAND_BY;
                  break;
                case SamplingUnitStatusMessage::ERROR:
                  logger->info("Unit " + unitID + " reported ERROR");
                  samplingUnit.status = SamplerStatus::UNIT_ERROR;
                  break;
            }
        }
        catch(const std::out_of_range& ex){
            logger->error("Status update message received from unknown unit " + unitID);
        }
    };

}

void SurfboardMainUnit::sendCommand(SamplingUnitRep& unit, ControlUnitCommand command){
    unsigned long current = millis();
    if((current - unit.lastCommandSentMillis) < COMMAND_SEND_MIN_INTERVAL_MILLIS){
        return;
    }
    try{
        std::map<string, string> commandParams;
        switch(command){
            case ControlUnitCommand::START_SAMPLING:
                commandParams["TIMESTAMP"] = to_string(currentSamplingSession);
                logger->debug("Sending START_SAMPLING command to unit " + macToString(unit.mac));
                syncManager->sendCommand(ControlUnitCommand::START_SAMPLING, commandParams, unit.mac);
                break;
            case ControlUnitCommand::STOP_SAMPLING:
                logger->debug("Sending STOP_SAMPLING command to unit " + macToString(unit.mac));
                syncManager->sendCommand(ControlUnitCommand::STOP_SAMPLING, commandParams, unit.mac);
                break;
            case ControlUnitCommand::START_SAMPLE_FILES_UPLOAD:
                logger->debug("Sending START_SAMPLE_FILES_UPLOAD command to unit " + macToString(unit.mac));
                syncManager->sendCommand(ControlUnitCommand::START_SAMPLE_FILES_UPLOAD, commandParams, unit.mac);
                break;
            case ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD:
                logger->debug("Sending STOP_SAMPLE_FILES_UPLOAD command to unit " + macToString(unit.mac));
                syncManager->sendCommand(ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD, commandParams, unit.mac);
                break;
        }
        unit.lastCommandSentMillis = millis();
    }catch(ESPNowSyncError& error){
        unit.status = SamplerStatus::UNIT_ERROR;
        return;
    } 
}

void SurfboardMainUnit::loopFileUpload(){
    int finishedCount = 0;
    int uploadingUnitsErrorNum = 0;
    if(sampler->hasFilesToCloudUpload()){
        if(sampler->getStatus() != SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
            logger->debug("Error in internal sampler file upload");
            try{
              logger->debug("Trying to reconnect to wifi and retry again...");
              sampler->connect();
            }catch(WifiError& er){
              // ignore...
            }
            updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_PARTIAL_ERROR);
            uploadingUnitsErrorNum++;
        }else{
            logger->debug("Uploading next file batch...");
            try{
                sampler->uploadNextSamples();
            }catch(...){
              // try to reconnect to wifi
              sampler->disconnect();
              sampler->connect();
            }
            
        }
    }else{
        sampler->disconnect();
        //syncManager->disconnect();
        //syncManager->connect();
        finishedCount++;
    }

    std::map<string, SamplingUnitRep>::iterator it;
    for (it=samplingUnits.begin(); it!=samplingUnits.end(); it++) {
        SamplerStatus unitStatus = it->second.status;
        if(unitStatus == SamplerStatus::UNIT_ERROR){
            updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_PARTIAL_ERROR);
            uploadingUnitsErrorNum++;
        }
        if(!it->second.hasFilesToUpload){
            finishedCount++;
        }else if(unitStatus != SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
            sendCommand(it->second, ControlUnitCommand::START_SAMPLE_FILES_UPLOAD);
        }
    }

    if(finishedCount == samplingUnits.size() + 1){
          updateStatus(SystemStatus::SYSTEM_STAND_BY);
          unsigned long elapsedTime = (millis() - uploadStartTime)/1000;
          logger->info("File upload completed! Process ended in " + to_string(elapsedTime) + " seconds");
    }else if(uploadingUnitsErrorNum == 0){
        updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD);
    }
}

void SurfboardMainUnit::loopSampling(){
    if(sampler->getStatus() != SamplerStatus::UNIT_SAMPLING){
        sampler->startSampling(currentSamplingSession);
    }else{
        sampler->writeSensorsData();
    }
    
    std::map<string, SamplingUnitRep>::iterator it;
    for (it=samplingUnits.begin(); it!=samplingUnits.end(); it++) {
        SamplerStatus unitStatus = it->second.status;
        if(unitStatus == SamplerStatus::UNIT_ERROR){
            updateStatus(SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR);
        }
        if(unitStatus != SamplerStatus::UNIT_SAMPLING){
            sendCommand(it->second, ControlUnitCommand::START_SAMPLING);
        }
    }
}

void SurfboardMainUnit::loopStandby(){
    if(sampler->getStatus() == SamplerStatus::UNIT_SAMPLING){
        sampler->stopSampling();
    }else if(sampler->getStatus() == SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
        sampler->disconnect();
    }
    
    std::map<string, SamplingUnitRep>::iterator it;
    for (it=samplingUnits.begin(); it!=samplingUnits.end(); it++) {
        SamplerStatus unitStatus = it->second.status;
        if(unitStatus == SamplerStatus::UNIT_SAMPLING){
           // logger->info("Unit " + it->first + " is still sampling, sending STOP_SAMPLING command");
            sendCommand(it->second, ControlUnitCommand::STOP_SAMPLING);
        }else if(unitStatus == SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
          //  logger->info("Unit " + it->first + " is still uploading sample files, sending STOP_SAMPLING command");
            sendCommand(it->second, ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD);
        }
        //else if(unitStatus == SamplerStatus::UNIT_ERROR){
            //sendCommand(it->second, ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD);
        //}
    }
}

void SurfboardMainUnit::loopDiscoverDisconnected(){
    unsigned long current = millis();
    std::map<string, SamplingUnitRep>::iterator it;
    for (it=samplingUnits.begin(); it!=samplingUnits.end(); it++) {
        if((current - it->second.lastStatusUpdateMillis) >= MAX_STATUS_UPDATE_DELAY && it->second.status != SamplerStatus::UNIT_ERROR){
            logger->info("Unit " + it->first + " is disconnected!");
            it->second.status = SamplerStatus::UNIT_ERROR;
        }
    }
}
