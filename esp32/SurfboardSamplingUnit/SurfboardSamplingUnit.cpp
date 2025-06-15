#include "SurfboardSamplingUnit.h"

SurfboardSamplingUnit::SurfboardSamplingUnit(WirelessHandler* wirelessHandler, SamplingUnitSyncManager *syncManager, SDCardHandler *sdCardHandler, Sampler *sampler, Logger *logger){
    this->syncManager=syncManager;
    this->sdCardHandler=sdCardHandler;
    this->sampler=sampler;
    this->wirelessHandler = wirelessHandler;
    this->logger = logger;
    lastStatusReportTime = 0;
}

void SurfboardSamplingUnit::addSensor(SensorBase *sensor){
    sampler->addSensor(sensor);
}

SamplerStatus SurfboardSamplingUnit::getStatus(){
    SamplerStatus stat = sampler->getStatus();
    return stat;
}

void SurfboardSamplingUnit::handleNextCommand(){
  // todo: add status report here after each command handling
    try{
        CommandMessage command = syncManager->getNextCommand();
        SamplerStatus samp_status= sampler->getStatus();
        switch(command.command){
            case ControlUnitCommand::START_SAMPLING:
                try{
                    int timestamp = stoi(command.params["TIMESTAMP"]);
                    if(sampler->getStatus()==UNIT_ERROR || sampler->getStatus()==UNIT_STAND_BY )
                    {
                        sampler->startSampling(timestamp);
                        reportStatus(SamplingUnitStatusMessage::SAMPLING,true);
                    } 
                    if(sampler->getStatus()==UNIT_SAMPLE_FILES_UPLOAD)
                    {
                        sampler->disconnect();
                        sampler->startSampling(timestamp);
                        reportStatus(SamplingUnitStatusMessage::SAMPLING,true);
                    }
                }catch(const exception& ex){
                    logger->error("Invalid command params");
                    reportStatus(SamplingUnitStatusMessage::ERROR,true);                  
                    return;
                }
                break;
            case ControlUnitCommand::STOP_SAMPLING:
                if(samp_status == SamplerStatus::UNIT_SAMPLING || samp_status == SamplerStatus::UNIT_ERROR)
                {
                    sampler->stopSampling();
                    reportStatus(SamplingUnitStatusMessage::STAND_BY,true);
                    return;
                }
                else if(samp_status == SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD)
                {
                    reportStatus(SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD,true);
                    return;
                }
                else if(samp_status == SamplerStatus::UNIT_STAND_BY)
                {
                    reportStatus(SamplingUnitStatusMessage::STAND_BY,true);
                    return;
                }
                break;        
            case ControlUnitCommand::START_SAMPLE_FILES_UPLOAD:
                if(sampler->hasFilesToCloudUpload()){
                    if(sampler->getStatus() == SamplerStatus::UNIT_SAMPLING)
                    {
                        sampler->stopSampling();
                    }
                    if(sampler->getStatus() != SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
                        logger->debug("Uploading internal sampler data started");
                        try{
                            sampler->connect();
                            reportStatus(SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD,true);
                        }
                        catch(WifiError& er){
                            reportStatus(SamplingUnitStatusMessage::ERROR,true);    
                            return;
                        }
                    }
                }
                else{
                    reportStatus(SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD_COMPLETE,true);    
                    return;
                }
                break;
            case ControlUnitCommand::STOP_SAMPLE_FILES_UPLOAD:
                if( samp_status == SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD || samp_status == SamplerStatus::UNIT_ERROR )
                {
                    sampler->disconnect();
                    reportStatus(SamplingUnitStatusMessage::STAND_BY,true);  
                    return;
                }
                else if(samp_status == SamplerStatus::UNIT_SAMPLING)
                {
                    reportStatus(SamplingUnitStatusMessage::SAMPLING,true);
                    return;
                }
                else if(samp_status == SamplerStatus::UNIT_STAND_BY)
                {
                    reportStatus(SamplingUnitStatusMessage::STAND_BY,true);
                    return;
                }
                break;
        }
    }catch(NotReadyError& err){
        // ignore, no command received yet
    }

}

void SurfboardSamplingUnit::loopSampling(){
    sampler->writeSensorsData();
}

void SurfboardSamplingUnit::loopFileUpload(){
    if(sampler->hasFilesToCloudUpload()){
        if(sampler->getStatus() != SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD){
            logger->debug("Error in internal sampler file upload");
        }else{
            logger->debug("Uploading next file batch...");
            sampler->uploadNextSamples();
        }
    }
    else{
        reportStatus(SamplingUnitStatusMessage::SAMPLE_FILES_UPLOAD_COMPLETE,true);
    }
}

void SurfboardSamplingUnit::reportStatus(SamplingUnitStatusMessage status_message , bool force ){
    unsigned long currentTime = millis(); 
    SamplerStatus currentStatus = sampler->getStatus();
    if ( (currentTime - lastStatusReportTime >= STATUS_REPORT_DELAY_MILLIS) || force ) 
    {
        syncManager->reportStatus(status_message);
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