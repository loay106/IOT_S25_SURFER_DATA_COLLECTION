#include "SensorBase.h"

SensorBase::SensorBase(Logger *logger, SDCardHandler *sdcardHandler, String model){
    this->logger = logger;
    this->sdcardHandler = sdcardHandler;
    this->model = model;
    samplingFileName = nullptr;
    sampleBuffer = new string("");
    samplesCount=0;
    samplingStartMillis=0;
}

String SensorBase::getModel(){
    return model;
}

void SensorBase::startSampling(string outputFilePath){
    samplingFileName = new string(outputFilePath);
    string* temp = sampleBuffer;
    temp = nullptr;
    sampleBuffer = new string("");
    delete temp;
    samplesCount=0;
    samplingStartMillis=millis();
    enableSensor();
}

void SensorBase::stopSampling(){
    flushSamplesBuffer(true);
    delete samplingFileName;
    samplingFileName = nullptr;
    unsigned long timeElapsed = (millis() - samplingStartMillis)/1000;
    int rate = samplesCount/timeElapsed;
    String message = String("Wrote ") + String(samplesCount) + " samples in " + String(timeElapsed) + " seconds. Sensor's rate: " + String(rate) + " Hz";
    logger->info(message);
    samplesCount=0;
    disableSensor();
}

void SensorBase::writeSamples(){
    if(!samplingFileName){
        logger->error(F("samplingFileName is empty!"));
        return;
    }
    try{
        string sample = getSample();
        sampleBuffer->append(sample);
        samplesCount++;
        if(sampleBuffer->length() >= MAX_SAMPLES_BUFFER_LENGTH){
            flushSamplesBuffer(false);
        }else{
            sampleBuffer->append("|");
        }
    }catch(NotReadyError& err){
        return;
    }
}

void SensorBase::flushSamplesBuffer(bool isLastLine){
    string* temp = sampleBuffer;
    if(isLastLine){
        if (!temp->empty()) {
            temp->pop_back();
        }
    }
    sampleBuffer = new string();
    sdcardHandler->writeData(*samplingFileName, temp->c_str());
    delete temp;
}