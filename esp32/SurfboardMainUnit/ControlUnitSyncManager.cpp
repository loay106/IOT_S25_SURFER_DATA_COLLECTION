#include "ControlUnitSyncManager.h"

ControlUnitSyncManager* ControlUnitSyncManager::instance = nullptr;
Logger* ControlUnitSyncManager::logger = Logger::getInstance();
queue<StatusUpdateMessage> ControlUnitSyncManager::statusUpdateQueue;
SemaphoreHandle_t ControlUnitSyncManager::queueMutex  = xSemaphoreCreateMutex();

void ControlUnitSyncManager::sendESPNowCommand(const ControlUnitCommand& command,const std::map<String,String>& params, uint8_t samplingUnitMac[6]){
    String message = serializeCommand(command, params);
    esp_err_t result = esp_now_send(samplingUnitMac, (uint8_t *) message.c_str(), message.length());
    if (result != ESP_OK) {
        ControlUnitSyncManager::logger->error(F("Failed to send command"));
        throw ESPNowSyncError();
    }
}

void ControlUnitSyncManager::broadcastESPNowCommand(const ControlUnitCommand& command,const std::map<String,String>& params){
    if(ControlUnitSyncManager::peers.size() == 0){
        return;
    }
    String message = serializeCommand(command, params);
    esp_err_t result = esp_now_send(nullptr, (uint8_t *) message.c_str(), message.length());
    if (result != ESP_OK) {
        ControlUnitSyncManager::logger->error(F("Failed to send command"));
        throw ESPNowSyncError();
    } 
}

void ControlUnitSyncManager::sendWifiStopFileUploadCommand(const String& unitMac){
  String hostname = getHostname(unitMac, false);
  String url = "http://" + hostname + ".local/stop";
  HTTPClient http;
  http.begin(url);
  http.setTimeout(10);
  logger->info("Sending stop upload to host " + hostname);
  int httpCode = http.POST("");
  http.end();
  if (httpCode == 204) {
    logger->info(hostname + " received command correctly!");
  }else{
    throw WifiError();
  }
}

void ControlUnitSyncManager::pingServerWifi(const String& unitMac){
  String hostname = getHostname(unitMac, false);
  String url = "http://" + hostname + ".local/ping";
  HTTPClient http;
  http.begin(url);
  http.setTimeout(10);
  logger->info("Sending stop upload to host " + hostname);
  int httpCode = http.GET("");
  http.end();
  if (httpCode == 204) {
    logger->info(hostname + " received command correctly!");
  }else{
    throw WifiError();
  }
}

bool ControlUnitSyncManager::hasStatusUpdateMessages(){
    return !ControlUnitSyncManager::statusUpdateQueue.empty();
}

void ControlUnitSyncManager::addStatusUpdateMessage(StatusUpdateMessage msg) {
    // Take the mutex before accessing the queue
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) == pdTRUE) {
        statusUpdateQueue.push(msg); // Add the message to the queue
        xSemaphoreGive(queueMutex); // Release the mutex
    } else {
        // Log an error if the mutex could not be acquired (unlikely in normal conditions)
        ControlUnitSyncManager::logger->error(F("Deadlock in queue mutex"));
    }
}

StatusUpdateMessage ControlUnitSyncManager::popStatusUpdateMessage() {
    StatusUpdateMessage msg;
    if (xSemaphoreTake(ControlUnitSyncManager::queueMutex, portMAX_DELAY) == pdTRUE) {
        msg = statusUpdateQueue.front(); // Get the front message
        statusUpdateQueue.pop();   
        xSemaphoreGive(ControlUnitSyncManager::queueMutex); // Release the mutex
    } else {
        ControlUnitSyncManager::logger->error(F("Deadlock in queue mutex"));
    }
    return msg;
}

void ControlUnitSyncManager::processReceivedMessages(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
    try{
        SamplingUnitStatusMessage status = deserializeStatusUpdateMsg(incomingData, len);
                // Create StatusUpdateMessage
        StatusUpdateMessage statusMessage;
        memcpy(statusMessage.from, mac_addr, 6);
        statusMessage.status = status;
        ControlUnitSyncManager::addStatusUpdateMessage(statusMessage);
    }catch(InvalidSyncMessage& err){
        logger->error(F("Invalid status update message received!"));
        return;
    }
}
