#include "ControlUnitSyncManager.h"

ControlUnitSyncManager* ControlUnitSyncManager::instance = nullptr;
Logger* ControlUnitSyncManager::logger = Logger::getInstance();
queue<StatusUpdateMessage> ControlUnitSyncManager::statusUpdateQueue;
SemaphoreHandle_t ControlUnitSyncManager::queueMutex  = xSemaphoreCreateMutex();
vector<esp_now_peer_info_t*> ControlUnitSyncManager::peers;

void ControlUnitSyncManager::init(uint8_t samplingUnits[][6], int samplingUnitsNum, int channel){
    WiFi.mode(WIFI_STA);

    this->channel = channel;

    for (int i = 0; i < samplingUnitsNum; i++) {
        esp_now_peer_info_t* peerInfo = new esp_now_peer_info_t();
        memcpy(peerInfo->peer_addr, samplingUnits[i], 6);
        peerInfo->channel = channel;
        peerInfo->encrypt = false;
        ControlUnitSyncManager::peers.push_back(peerInfo);
    }
}

void ControlUnitSyncManager::connect(){
    if(isConnected){
        return;
    }
    WiFi.mode(WIFI_STA);
    if (esp_now_init() == ESP_OK) {
        ControlUnitSyncManager::logger->info("ESP Now init success! Connected to channel " + to_string(channel));
    }else {
        ControlUnitSyncManager::logger->error("ESPNow init failed!");
        throw ESPNowSyncError();
    }

    for(esp_now_peer_info_t* peerInfo: peers){
        if (esp_now_add_peer(peerInfo) != ESP_OK) {
            throw ESPNowSyncError();
        }
    }

    esp_now_register_recv_cb(ControlUnitSyncManager::processReceivedMessages);
    isConnected=true;
}

void ControlUnitSyncManager::disconnect(){
    esp_now_deinit();
    isConnected=false;
}

void ControlUnitSyncManager::sendCommand(const ControlUnitCommand& command,const std::map<string,string>& params, uint8_t samplingUnitMac[6]){
    string message = serializeCommand(command, params);
    esp_err_t result = esp_now_send(samplingUnitMac, (uint8_t *) message.c_str(), message.length());
    if (result != ESP_OK) {
        ControlUnitSyncManager::logger->error("Failed to send command");
        throw ESPNowSyncError();
    }
}

void ControlUnitSyncManager::broadcastCommand(const ControlUnitCommand& command,const std::map<string,string>& params){
    if(ControlUnitSyncManager::peers.size() == 0){
        return;
    }
    string message = serializeCommand(command, params);
    esp_err_t result = esp_now_send(nullptr, (uint8_t *) message.c_str(), message.length());
    if (result != ESP_OK) {
        ControlUnitSyncManager::logger->error("Failed to send command");
        throw ESPNowSyncError();
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
        ControlUnitSyncManager::logger->error("Deadlock in queue mutex");
    }
}

StatusUpdateMessage ControlUnitSyncManager::popStatusUpdateMessage() {
    StatusUpdateMessage msg;
    if (xSemaphoreTake(ControlUnitSyncManager::queueMutex, portMAX_DELAY) == pdTRUE) {
        msg = statusUpdateQueue.front(); // Get the front message
        statusUpdateQueue.pop();   
        xSemaphoreGive(ControlUnitSyncManager::queueMutex); // Release the mutex
    } else {
        ControlUnitSyncManager::logger->error("Deadlock in queue mutex");
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
        logger->error("Invalid status update message received!");
        return;
    }
}
