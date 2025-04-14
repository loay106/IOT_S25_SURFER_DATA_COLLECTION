#include "SensorBase.h"

SensorBase::SensorBase(Logger *logger, SDCardHandler *sdcardHandler, string model){
    this->logger = logger;
    this->sdcardHandler = sdcardHandler;
    this->model = model;
    samplingFileName = nullptr;
    sampleBuffer = new string("");
    samplesCount=0;
    samplingStartMillis=0;
}

string SensorBase::getModel(){
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
    string message = "Wrote " + to_string(samplesCount) + " samples in " + to_string(timeElapsed) + " seconds. Sensor's rate: " + to_string(rate) + " Hz";
    logger->info(message);
    samplesCount=0;
    disableSensor();
}

void SensorBase::writeSamples(){
    if(!samplingFileName){
        logger->error("samplingFileName is empty!");
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