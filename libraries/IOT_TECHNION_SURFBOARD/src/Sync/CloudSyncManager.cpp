#include "CloudSyncManager.h"

CloudSyncManager::CloudSyncManager(Logger* logger, WifiHandler *wifiHandler, String unitMac){
    this->wifiHandler=wifiHandler;
    this->logger = logger;
    this->sampleUploadEndpoint = "https://me-west1-surfer-data-project.cloudfunctions.net/api-1/addSamples";    
    this->unitMac = unitMac;
}

void CloudSyncManager::init() {}

void CloudSyncManager::connect(){
    try{
        logger->info("Connecting to wifi...");
        wifiHandler->connect();
        logger->info("Successfully connected to Wifi.");
        httpClient.begin(sampleUploadEndpoint);
        httpClient.addHeader("Content-Type", "application/json");
        logger->info("HTTP connection initiated Successfully.");
    }catch(WifiError& err){
        logger->debug("Wifi connection failed!");
        throw;
    }
}

void CloudSyncManager::disconnect() {
    httpClient.end();
    logger->info("HTTP connection finished Successfully.");
    wifiHandler->disconnect();
    logger->info("Successfully disconnected from Wifi.");
}

static String sampleArrayJson1 = "[";
static int sampleCount1 = 0;



void CloudSyncManager::sendBuffer(String timestamp, String sensorID, String sensorModel) {
    if (sampleCount1 == 0) return;

    if (sampleArrayJson1.endsWith(",")) {
        sampleArrayJson1 = sampleArrayJson1.substring(0, sampleArrayJson1.length() - 1);
    }
    sampleArrayJson1 += "]";

    if (ESP.getFreeHeap() < 19000) {
        logger->error("Low memory! Stopping uploads.");
        return;
    }
//    logger->info("Heap before sendBuffer: " + to_string(ESP.getFreeHeap()));

    String jsonData;
    jsonData.reserve(19000);

    jsonData = "{\"timestamp\": \"";
    jsonData += timestamp;
    jsonData += "\",\"sensorID\": \"";
    jsonData += sensorID;
    jsonData += "\",\"sensorModel\": \"";
    jsonData += sensorModel;
    jsonData += "\", \"unitMac\": \"";
    jsonData += wifiHandler->getUnitMac();
    jsonData += "\",\"samples\": ";
    jsonData += sampleArrayJson1;
    jsonData += "}";

    int retries = 3;
    int httpResponseCode = -1;

    while (retries-- > 0) {
        httpResponseCode = httpClient.POST(jsonData);
        if (httpResponseCode == 200) break;

        logger->error("Retrying... ");
        delay(300);
    }

    if (httpResponseCode == 200) {
        logger->info("Data sent successfully!");
    } else {
        logger->error("Error sending data: " + to_string(httpResponseCode));
        throw CloudSyncError();
    }

    // Reset JSON buffer
    sampleArrayJson1 = "[";
    sampleCount1 = 0;
}

void CloudSyncManager::uploadSamples(String timestamp, String sensorID, String sensorModel, String samples, int* fileSampleIndex) {
    int startIndex = 0;
    int endIndex = samples.indexOf('|');

    while (endIndex > 0) {
        String sample = samples.substring(startIndex, endIndex);
        String jsonSample = "{\"sample\": \"" + sample + "\", \"sample_index\": " + String(*fileSampleIndex) + "},";
        *fileSampleIndex += 1;

        sampleArrayJson1 += jsonSample;
        sampleCount1++;
        if (sampleArrayJson1.length() > 18000) {
            sendBuffer(timestamp, sensorID, sensorModel);
        }
        startIndex = endIndex + 1;
        endIndex = samples.indexOf('|', startIndex);
    }

    // Add last sample
    String lastSample = samples.substring(startIndex);
    if (!lastSample.isEmpty()) {
        String jsonLastSample = "{\"sample\": \"" + lastSample + "\", \"sample_index\": " + String(*fileSampleIndex) + "},";
        *fileSampleIndex += 1;

        sampleArrayJson1 += jsonLastSample;
        sampleCount1++;
    }
}

void CloudSyncManager::uploadLastSamples(String timestamp, String sensorID, String sensorModel) {
    if (sampleCount1 > 0) {
        sendBuffer(timestamp, sensorID, sensorModel);
    }
}

