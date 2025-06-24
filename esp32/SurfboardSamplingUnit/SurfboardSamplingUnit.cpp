#include "SurfboardSamplingUnit.h"

const unsigned long MAX_WIFI_CONNECT_TRY_MILLIS = 30000;

SurfboardSamplingUnit::SurfboardSamplingUnit(WirelessHandler* wirelessHandler, SamplingUnitSyncManager *syncManager, SDCardHandler *sdCardHandler, Sampler *sampler, Logger *logger, DataCollectorServer* _server){
    this->syncManager=syncManager;
    this->sdCardHandler=sdCardHandler;
    this->sampler=sampler;
    this->wirelessHandler = wirelessHandler;
    this->logger = logger;
    this->status = SamplerStatus::UNIT_STAND_BY;
    this->WIFI_PASSWORD = "";
    this->WIFI_SSID = "";
    this->server = _server;
    lastStatusReportTime = 0;
}

void SurfboardSamplingUnit::addSensor(SensorBase *sensor){
    sampler->addSensor(sensor);
}

void SurfboardSamplingUnit::handleNextCommand(){
    try{
        CommandMessage command = syncManager->getNextCommand();
        logger->info("Received new command: " + command_to_string(command.command) + String(" with ") + String(command.params.size()) + " params");
        logger->info("Current status: " + sampler_status_to_string(this->status));
        switch(command.command){
            case ControlUnitCommand::START_SAMPLING:
                try{
                    this->currentSamplingSession = strtoul(command.params["TIMESTAMP"].c_str(), nullptr, 10);
                    this->status = SamplerStatus::UNIT_SAMPLING;
                    logger->info("Unit status changed to " + sampler_status_to_string(this->status));
                    logger->info("Current sampling session " + String(this->currentSamplingSession));
                    reportStatus(true);
                }catch(const exception& ex){
                    logger->error("Invalid command params");
                    reportStatus(true);                  
                    return;
                }
                break;
            case ControlUnitCommand::STOP_SAMPLING:
            case ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD:
                try{
                    this->status = SamplerStatus::UNIT_STAND_BY;
                    logger->info("Unit status changed to " + sampler_status_to_string(this->status));
                    reportStatus(true);
                }catch(const exception& ex){
                    logger->error("Invalid command params");
                    reportStatus(true);                  
                    return;
                }
                break;    
            case ControlUnitCommand::START_SAMPLE_FILES_UPLOAD:
                try{
                    WIFI_SSID = command.params["WIFI_SSID"];
                    WIFI_PASSWORD = command.params["WIFI_PASSWORD"];
                    this->status = SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD;
                    logger->info("Unit status changed to " + sampler_status_to_string(this->status));
                    reportStatus(true);
                }catch(const exception& ex){
                    logger->error("Invalid command params");
                    reportStatus(true);                  
                    return;
                }
                break;    
        }
    }catch(NotReadyError& err){
        // ignore, no command received yet
    }

}

void SurfboardSamplingUnit::loopSampling(){
    if (!sampler->isSampling()) {
      sampler->startSampling(this->currentSamplingSession);
    } else {
      sampler->writeSensorsData();
    }
    
    if(wirelessHandler->getCurrentMode() != WirelessHandler::MODE::ESP_NOW){
      wirelessHandler->switchToESPNow();
    }
    if(server->isServerRunning()){
      server->stop();
    }
    
}

void SurfboardSamplingUnit::loopStandBy(){
    if (sampler->isSampling()) {
      sampler->stopSampling();
    } 

    if(wirelessHandler->getCurrentMode() != WirelessHandler::MODE::ESP_NOW){
      wirelessHandler->switchToESPNow();
    }

    if(server->isServerRunning()){
      server->stop();
    }
}

void SurfboardSamplingUnit::loopFileUpload(){
    if(wirelessHandler->getCurrentMode() != WirelessHandler::MODE::WIFI){
      wirelessHandler->switchToWifi(WIFI_SSID, WIFI_PASSWORD);
      wifi_connection_start_time = millis();
      return;
    }
    if(!wirelessHandler->isConnected()){
      if((millis() - wifi_connection_start_time) > MAX_WIFI_CONNECT_TRY_MILLIS){
        logger->error("Wifi connection retry timed out...Switching to standby mode until a new command is received...");
        wirelessHandler->switchToESPNow();
        status = SamplerStatus::UNIT_STAND_BY;
      }
      return;
    }

    if(!server->isServerRunning()){
      server->begin();
      return;
    }

    if(server->isStopRequestReceived()){
      server->stop();
      status = SamplerStatus::UNIT_STAND_BY;
    }
}



SamplerStatus SurfboardSamplingUnit::getStatus(){
    SamplerStatus current = status; 
    return current;
}

void SurfboardSamplingUnit::reportStatus(bool force){
    unsigned long currentTime = millis(); 
    if ((currentTime - lastStatusReportTime >= STATUS_REPORT_DELAY_MILLIS) || force) {
        syncManager->reportStatus(this->status);
        lastStatusReportTime = currentTime;     
    }
}

void SurfboardSamplingUnit::loopComponents(){
  try{
      wirelessHandler->loop();
  }catch(...){
    logger->error("Wireless handler error..");
  } 
}