#include "Sampler.h"


Sampler::Sampler(Logger* logger, SDCardHandler* sdCardHandler){
     this->logger = logger;
     this->sdCardHandler = sdCardHandler;
     this->_isSampling = false;
}

void Sampler::addSensor(SensorBase *sensor){
    sensors.push_back(sensor);
}

void Sampler::init(){
    try{
        sdCardHandler->createFolder("/samplings");
    }catch(SDCardError& err){
        logger->error("Failed to create samplings folder");
        throw InitError();
    }
}

void Sampler::startSampling(int timestamp){
    string curretTimestamp = to_string(timestamp);
    logger->info("Sampling started!");
    for(int i=0;i<sensors.size(); i++){
        string filePath = "/samplings/" + curretTimestamp + "_" + to_string(i) + "_" + sensors[i]->getModel();
        sensors[i]->startSampling(filePath);
    }    
    _isSampling=true;
}


void Sampler::stopSampling(){
    logger->info("Sampling stopped!");
    for(int i=0; i<sensors.size(); i++){
        string message = "Sensor id=" + to_string(i) + ", model=" + sensors[i]->getModel() + " stopped!";
        logger->info(message);
        sensors[i]->stopSampling();
    }
    _isSampling=false;
 }

 bool Sampler::isSampling(){
     return this->_isSampling;
 }

void Sampler::writeSensorsData(){
     for(int i= 0; i< Sampler::sensors.size(); i++){
         sensors[i]->writeSamples();
     }
}
