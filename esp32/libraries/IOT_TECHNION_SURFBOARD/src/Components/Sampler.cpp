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

void Sampler::startSampling(unsigned long timestamp){
    logger->info(F("Sampling started!"));
    for(int i=0;i<sensors.size(); i++){
        String filePath = String("/samplings/") + String(timestamp) + "_" + String(i) + "_" + sensors[i]->getModel();
        sensors[i]->startSampling(filePath);
    }    
    _isSampling=true;
}


void Sampler::stopSampling(){
    logger->info(F("Sampling stopped!"));
    for(int i=0; i<sensors.size(); i++){
        String message = String("Sensor id=") + String(i) + ", model=" + sensors[i]->getModel() + " stopped!";
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
