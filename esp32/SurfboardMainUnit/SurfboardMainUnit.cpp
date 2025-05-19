#include "SurfboardMainUnit.h"
#include <HTTPClient.h>

SurfboardMainUnit::SurfboardMainUnit(ControlUnitSyncManager *syncManager, RTCTimeHandler* timeHandler, RGBStatusHandler* statusLighthandler, ButtonHandler *buttonHandler, Logger *logger, Sampler* sampler, SDCardHandler* sdCardHandler,WifiHandler* wifiHandler, string _wifi_ssid, string _wifi_password, DataCollectorServer* server){
    this->logger = logger;
    this->syncManager = syncManager;
    this->timeHandler = timeHandler;
    this->statusLighthandler = statusLighthandler;
    this->buttonHandler = buttonHandler;
    this->logger = logger;
    this->sampler = sampler;
    this->sdCardHandler = sdCardHandler;
    this->WIFI_SSID = _wifi_ssid;
    this->WIFI_PASSWORD = _wifi_password;
    this->wifiHandler = wifiHandler;
    this->server = server;
    status = SystemStatus::SYSTEM_STARTING;
    currentSamplingSession = 0;
}

void SurfboardMainUnit::init(uint8_t samplingUnitsAdresses[][6], int samplingUnitsNum) {
    for(int i=0;i<samplingUnitsNum;i++){
        SamplingUnitRep samplingUnit;
        memcpy(samplingUnit.mac, samplingUnitsAdresses[i], 6);
        samplingUnit.status = SamplerStatus::UNIT_STAND_BY;
        samplingUnit.lastCommandSentMillis = 0;
        samplingUnit.lastStatusUpdateMillis = 0;
        String macString = macToString(samplingUnit.mac);
        samplingUnit.mDNSHostname = getHostname(macString, false) + ".local";
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
    if (wifiHandler->isConnected()) {
        wifiHandler->disconnect();
        delay(500);
    }
    if (!syncManager->isESPNowConnected()) {
        syncManager->connect();
        delay(300);
    }

    currentSamplingSession = timeHandler->getCurrentTimestamp();
    std::map<string, string> samplingParams;
    samplingParams["TIMESTAMP"] = to_string(currentSamplingSession);
    try {
        syncManager->broadcastCommand(ControlUnitCommand::START_SAMPLING, samplingParams);
    } catch (ESPNowSyncError& error) {
        logger->error("Failed to send command to sampling units! Try again!");
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
    if (wifiHandler->isConnected()) {
        wifiHandler->disconnect();
        delay(500);
    }
    if (!syncManager->isESPNowConnected()) {
        syncManager->connect();
        delay(300);
    }
    try {
        std::map<string, string> params;
        syncManager->broadcastCommand(ControlUnitCommand::STOP_SAMPLING, params);
    } catch (ESPNowSyncError& error) {
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
    logger->info("File upload starting...");

    if (wifiHandler->isConnected()) {
        wifiHandler->disconnect();
        delay(500);
    }
    if (!syncManager->isESPNowConnected()) {
        syncManager->connect();
        delay(300);
    }

    std::map<string, string> params;
    params["WIFI_SSID"] = WIFI_SSID;
    params["WIFI_PASSWORD"] = WIFI_PASSWORD;

    for (int i = 0; i < 3; i++) {
        try {
            syncManager->broadcastCommand(ControlUnitCommand::START_SAMPLE_FILES_UPLOAD, params);
        } catch (ESPNowSyncError& error) {
            logger->error("Failed to send command to sampling units! Try again!");
        }
    }

    unsigned long current = millis();
    std::map<String, SamplingUnitRep>::iterator it;
    for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
        it->second.lastCommandSentMillis = current;
    }

    delay(200); // give some time for units to respond

    if (syncManager->isESPNowConnected()) {
        syncManager->disconnect();
    }
    if (!wifiHandler->isConnected()) {
        wifiHandler->connect();
    }

    server->begin();
    logger->info(String("File upload started!"));
    updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD);
}

bool SurfboardMainUnit::sendStopUploadToSamplingUnitDataServer(String hostname) {
    if (!wifiHandler->isConnected()) {
        return false;
    }

    String url = "http://" + hostname + "/stop";
    HTTPClient http;
    http.begin(url);
    http.setTimeout(5);

    logger->info("Sending stop upload to host " + hostname);

    int httpCode = http.POST("");
    http.end();

    if (httpCode == 204) {
        logger->info(hostname + " received command correctly!");
        return true;
    } else {
        logger->error("Failed to send command stop uploading to unit " + hostname);
        return false;
    }
}

void SurfboardMainUnit::stopSampleFilesUpload() {
    updateStatus(SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD_STOPPING);
    logger->info(String("Stopping file upload..."));

    if (syncManager->isESPNowConnected()) {
        syncManager->disconnect();
        delay(300);
    }
    if (!wifiHandler->isConnected()) {
        wifiHandler->connect();
        delay(500);
    }

    unsigned long current = millis();
    std::map<String, SamplingUnitRep>::iterator it;
    for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
        bool result = sendStopUploadToSamplingUnitDataServer(it->second.mDNSHostname);
        if (result) {
            it->second.lastCommandSentMillis = current;
            it->second.status = SamplerStatus::UNIT_STAND_BY;
        }
    }

    updateStatus(SystemStatus::SYSTEM_STAND_BY);
}


SystemStatus SurfboardMainUnit::getStatus(){
    SystemStatus res = status;
    return res;
}

void SurfboardMainUnit::handleButtonPress() {
    ButtonPressType press = buttonHandler->getLastPressType();
    if (press != ButtonPressType::NO_PRESS) {
        switch (status) {
            case SystemStatus::SYSTEM_SAMPLING:
            case SystemStatus::SYSTEM_SAMPLING_PARTIAL_ERROR:
                logger->debug(String("Soft or long press detected while sampling"));
                stopSampling();
                break;

            case SystemStatus::SYSTEM_SAMPLE_FILE_UPLOAD:
                logger->debug(String("Soft or long press detected while file uploading"));
                stopSampleFilesUpload();
                break;

            case SystemStatus::SYSTEM_STAND_BY:
                if (press == ButtonPressType::SOFT_PRESS) {
                    logger->debug(String("Soft press detected while on standby"));
                    startSampling();
                } else {
                    logger->debug(String("Long press detected while on standby"));
                    startSampleFilesUpload();
                }
                break;
        }
        delay(100); // give some time for status messages
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


void SurfboardMainUnit::sendCommand(SamplingUnitRep& unit, ControlUnitCommand command) {
    if (wifiHandler->isConnected()) {
        wifiHandler->disconnect();
        delay(500);
    }

    if (!syncManager->isESPNowConnected()) {
        syncManager->connect();
        delay(300);
    }

    unsigned long current = millis();
    if ((current - unit.lastCommandSentMillis) < COMMAND_SEND_MIN_INTERVAL_MILLIS) {
        return;
    }

    try {
        std::map<string, string> commandParams;
        String macStr = macToString(unit.mac);

        switch (command) {
            case ControlUnitCommand::START_SAMPLING:
                commandParams["TIMESTAMP"] = to_string(currentSamplingSession);
                logger->debug(String("Sending START_SAMPLING command to unit ") + macStr);
                syncManager->sendCommand(ControlUnitCommand::START_SAMPLING, commandParams, unit.mac);
                break;

            case ControlUnitCommand::STOP_SAMPLING:
                logger->debug(String("Sending STOP_SAMPLING command to unit ") + macStr);
                syncManager->sendCommand(ControlUnitCommand::STOP_SAMPLING, commandParams, unit.mac);
                break;

            case ControlUnitCommand::START_SAMPLE_FILES_UPLOAD:
                logger->debug(String("Sending START_SAMPLE_FILES_UPLOAD command to unit ") + macStr);
                syncManager->sendCommand(ControlUnitCommand::START_SAMPLE_FILES_UPLOAD, commandParams, unit.mac);
                break;

            case ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD:
                logger->debug(String("Sending STOP_SAMPLE_FILES_UPLOAD command to unit ") + macStr);
                syncManager->sendCommand(ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD, commandParams, unit.mac);
                break;
        }

        unit.lastCommandSentMillis = millis();
    } catch (ESPNowSyncError& error) {
        unit.status = SamplerStatus::UNIT_ERROR;
        return;
    }
}

bool SurfboardMainUnit::pingSamplingUnitDataServer(String hostname) {
    if (!wifiHandler->isConnected()) {
        return false;
    }

    String url = "http://" + hostname + "/ping";
    HTTPClient http;
    http.begin(url);
    http.setTimeout(5);

    logger->info(String("Sending ping to host ") + hostname);

    int httpCode = http.GET();
    http.end();

    if (httpCode == 204) {
        logger->info(hostname + String(" is alive and connected"));
        return true;
    } else {
        logger->error(String("Failed to ping unit ") + hostname);
        return false;
    }
}

 

void SurfboardMainUnit::loopFileUpload() {
    unsigned long current = millis();
    std::map<String, SamplingUnitRep>::iterator it;
    for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
        if ((current - it->second.lastStatusUpdateMillis) >= STATUS_REPORT_DELAY_MILLIS) {
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
            sendCommand(it->second, ControlUnitCommand::START_SAMPLING);
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
            // logger->info("Unit " + it->first + " is still sampling, sending STOP_SAMPLING command");
            // If uncommented:
            // logger->info(F("Unit ") + String(it->first.c_str()) + F(" is still sampling, sending STOP_SAMPLING command"));
            sendCommand(it->second, ControlUnitCommand::STOP_SAMPLING);
        }
    }
}

void SurfboardMainUnit::loopDiscoverDisconnected() {
    unsigned long current = millis();
    std::map<String, SamplingUnitRep>::iterator it;
    for (it = samplingUnits.begin(); it != samplingUnits.end(); it++) {
        if ((current - it->second.lastStatusUpdateMillis) >= MAX_STATUS_UPDATE_DELAY &&
            it->second.status != SamplerStatus::UNIT_ERROR) {
            logger->info(String("Unit ") + String(it->first.c_str()) + String(" is disconnected!"));
            it->second.status = SamplerStatus::UNIT_ERROR;
        }
    }
}

