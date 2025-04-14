#include "Sampler.h"


Sampler::Sampler(Logger* logger, SDCardHandler* sdCardHandler, CloudSyncManager* cloudSyncManager){
     status = SamplerStatus::UNIT_STAND_BY;
     this->logger = logger;
     this->sdCardHandler = sdCardHandler;
     this->cloudSyncManager = cloudSyncManager;
     this->hasFilesToUpload = false;
     this->fileSampleIndex = 0;
}

void Sampler::addSensor(SensorBase *sensor){
    sensors.push_back(sensor);
}

void Sampler::init(){
    try{
        sdCardHandler->createFolder("/samplings");
        if(sdCardHandler->listFilesInDir("/samplings").size() > 0){
            hasFilesToUpload = true;
        }
    }catch(SDCardError& err){
        logger->error("Failed to create samplings folder");
        status = SamplerStatus::UNIT_ERROR;
        throw InitError();
    }
}

void Sampler::startSampling(int timestamp){
    status = SamplerStatus::UNIT_SAMPLING;
    logger->info("Sampling started!");
    for(int i=0;i<sensors.size(); i++){
        // sample files have this format:
        // [TIMESTAMP]_[SENSOR_ID]_[SENSOR_MODEL]
        string filePath = "/samplings/" + to_string(timestamp) + "_" + to_string(i) + "_" + sensors[i]->getModel();
        sensors[i]->startSampling(filePath);
    }
    if(sensors.size() > 0){
        hasFilesToUpload=true;
    }
    
}


void Sampler::stopSampling(){
    logger->info("Sampling stopped!");
    for(int i=0; i<sensors.size(); i++){
        string message = "Sensor id=" + to_string(i) + ", model=" + sensors[i]->getModel() + " stopped!";
        logger->info(message);
        sensors[i]->stopSampling();
    }
    status = SamplerStatus::UNIT_STAND_BY;
 }

 SamplerStatus Sampler::getStatus(){
    SamplerStatus res = status;
    return res;
 }

 void Sampler::enterErrorState(){
     status = SamplerStatus::UNIT_ERROR;
 }

 void Sampler::connect(){
    try{
        if(!cloudSyncManager->isWifiConnected()){
            cloudSyncManager->connect();
        }
        status = SamplerStatus::UNIT_SAMPLE_FILES_UPLOAD;
    }catch(...){ 
        status = SamplerStatus::UNIT_ERROR;
        logger->error("Connecting to wifi failed!");
        throw;
    }
}

void Sampler::disconnect() {
    cloudSyncManager->disconnect();
    if (currentUploadFile) { 
        currentUploadFile.close();
        fileSampleIndex = 0;
    }
    status = SamplerStatus::UNIT_STAND_BY;
}


 bool Sampler::isConnected(){
    return cloudSyncManager->isWifiConnected();
 }

 void Sampler::uploadNextSamples(){
    if(!hasFilesToUpload){
        logger->info("No sample files to upload!");
        status = SamplerStatus::UNIT_STAND_BY;
        return; // finished
    }

    if(!isConnected()){
        connect();
        return;
    }

    String fileName;
    String timestamp;
    String sensorID;
    String sensorModel;

    if(!currentUploadFile){
        // try to open the next file
        File root;
        sdCardHandler->getFolder("/samplings",&root);
        currentUploadFile = root.openNextFile();
        fileSampleIndex=0;

        if(currentUploadFile){
            fileName = currentUploadFile.name();
            logger->info("Processing file: " + string(fileName.c_str()));
        }else{
            // finished uploading all *files*
            hasFilesToUpload=false;
            if(currentUploadFile){
                currentUploadFile.close();
                fileSampleIndex=0;
            }
            fileSampleIndex=0;
            status = SamplerStatus::UNIT_STAND_BY;
            logger->info("Finished uploading all sample files!");
            return;
        }
    }

    fileName = currentUploadFile.name();
    timestamp = fileName.substring(0, fileName.indexOf('_'));
    sensorID = fileName.substring(fileName.indexOf('_') + 1, fileName.lastIndexOf('_'));
    sensorModel = fileName.substring(fileName.lastIndexOf('_') + 1);

    if(currentUploadFile.available()){
        // file has lines to upload
        try {
            String line = currentUploadFile.readStringUntil('\n');
            line.trim();
            if (line.length() == 0){
                // skip this invalid line
            }else{
                cloudSyncManager->uploadSamples(timestamp, sensorID, sensorModel,line, &fileSampleIndex);
            }
        } catch (...) {
            logger->error("Failed to upload samples from CloudSyncManager");
            if(currentUploadFile){
                currentUploadFile.close();
                fileSampleIndex=0;
            }
            status = SamplerStatus::UNIT_ERROR;
        }
    }else{
        // current file finished uploading
        cloudSyncManager->uploadLastSamples(timestamp,sensorID,sensorModel);
        logger->info("Finished uploading file: "+ string(fileName.c_str()));
        currentUploadFile.close();
        // delete file *AFTER* it was uploaded to the cloud to free up SD card space
        sdCardHandler->deleteFile("/samplings/" + fileName); 
        fileSampleIndex=0;
    }
}



void Sampler::writeSensorsData(){
     for(int i= 0; i< Sampler::sensors.size(); i++){
         sensors[i]->writeSamples();
     }
}
