#include "SamplingUnitSyncManager.h"

SamplingUnitSyncManager* SamplingUnitSyncManager::instance = nullptr;
CommandMessage* SamplingUnitSyncManager::nextCommand = nullptr;
Logger* SamplingUnitSyncManager::logger = Logger::getInstance();

void SamplingUnitSyncManager::onDataReceivedCallback(const uint8_t *mac_addr, const uint8_t *incomingData, int len){
    try{
        CommandMessage cmd = deserializeCommand(incomingData, len);
        SamplingUnitSyncManager::setNextCommand(cmd);
    }catch(InvalidSyncMessage& err){
        SamplingUnitSyncManager::logger->error(F("Invalid command message received!"));
        return;
    }
}

void SamplingUnitSyncManager::reportStatus(SamplerStatus status){
    String message = serializeStatusUpdateMsg(status);
    esp_err_t result = esp_now_send(controlUnitMac, (uint8_t *) message.c_str(), message.length());
    if (result != ESP_OK) {
        SamplingUnitSyncManager::logger->error(F("Failed to report status!"));
    }
}

void SamplingUnitSyncManager::setNextCommand(CommandMessage cmd){
    // todo: use lock here and in getNextCommand?
    delete SamplingUnitSyncManager::nextCommand;
    SamplingUnitSyncManager::nextCommand = new CommandMessage();
    SamplingUnitSyncManager::nextCommand->command = cmd.command;
    SamplingUnitSyncManager::nextCommand->params = cmd.params;
}

CommandMessage SamplingUnitSyncManager::getNextCommand(){
    if(SamplingUnitSyncManager::nextCommand){
        CommandMessage command;
        command.command = SamplingUnitSyncManager::nextCommand->command;
        command.params = SamplingUnitSyncManager::nextCommand->params;
        delete SamplingUnitSyncManager::nextCommand;
        SamplingUnitSyncManager::nextCommand = nullptr;
        return command;
    }else{
        throw NotReadyError();
    }
}
