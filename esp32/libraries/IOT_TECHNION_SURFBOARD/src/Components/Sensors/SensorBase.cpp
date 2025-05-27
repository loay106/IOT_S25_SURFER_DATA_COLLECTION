#include "SensorBase.h"

SensorBase::SensorBase(Logger *logger, SDCardHandler *sdcardHandler, String model){
    this->logger = logger;
    this->sdcardHandler = sdcardHandler;
    this->model = model;
    samplingFileName = "";
    sampleBuffer = "";
    samplesCount=0;
    samplingStartMillis=0;
}

String SensorBase::getModel(){
    return model;
}

void SensorBase::startSampling(const String& outputFilePath){
    samplingFileName = String(outputFilePath);
    sampleBuffer = "";
    samplesCount=0;
    samplingStartMillis=millis();
    enableSensor();
}

void SensorBase::stopSampling(){
    flushSamplesBuffer(true);
    samplingFileName = "";
    unsigned long timeElapsed = (millis() - samplingStartMillis)/1000;
    int rate = 0;
    if(timeElapsed > 0){
        rate = samplesCount/timeElapsed;
    }
    String message = String("Wrote ") + String(samplesCount) + " samples in " + String(timeElapsed) + " seconds. Sensor's rate: " + String(rate) + " Hz";
    logger->info(message);
    samplesCount=0;
    disableSensor();
}

void SensorBase::writeSamples(){
    if(samplingFileName == ""){
        logger->error(F("samplingFileName is empty!"));
        return;
    }
    try{
        String sample = getSample();
        sampleBuffer += sample;
        samplesCount++;
        if(sampleBuffer.length() >= MAX_SAMPLES_BUFFER_LENGTH){
            flushSamplesBuffer(false);
        }else{
            sampleBuffer += "|";
        }
    }catch(NotReadyError& err){
        return;
    }
}

void SensorBase::flushSamplesBuffer(bool isLastLine){
    if (isLastLine && sampleBuffer.endsWith("|")) {
        sampleBuffer.remove(sampleBuffer.length() - 1);
    }
    sdcardHandler->writeData(samplingFileName, sampleBuffer.c_str());
    sampleBuffer = "";
}