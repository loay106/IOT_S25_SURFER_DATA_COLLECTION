#include "DataCollectorServer.h"
#include "../Utils/Adresses.h"

bool DataCollectorServer::stopFlag = false;

DataCollectorServer::DataCollectorServer(SDCardHandler *sdHandler, const String &macAddress, bool isMain) : server(80), sd(sdHandler) {
    hostname = getHostname(macAddress, isMain);
}

void DataCollectorServer::begin()
{
    if (serverStarted) return;

    Logger::getInstance()->info("Starting uploader server...");

    if (!MDNS.begin(hostname.c_str())) {
      Logger::getInstance()->error(F("Error setting up mDNS responder"));
      return;
    } else {
      Logger::getInstance()->info("mDNS responder started with hostname: " + hostname + ".local");
      MDNS.addService("_http", "_tcp", 80);
      MDNS.addService("_surferdata", "_tcp", 80);
      MDNS.addServiceTxt("_http", "_tcp", "device", hostname);
      MDNS.addServiceTxt("_surferdata", "_tcp", "device", hostname);
      Logger::getInstance()->debug(F("mDNS services registered."));
    }

    expose_list_samplings_endpoint();
    expose_download_endpoint();
    expose_validate_download_endpoint();
    expose_remove_all_samplings_endpoint();
    expose_ping_endpoint();
    expose_server_stop_endpoint();

    server.begin();
    serverStarted = true;
    Logger::getInstance()->info(F("Uploader server started on port 80"));
}

bool DataCollectorServer::isStopRequestReceived(){
    bool flag = stopFlag;
    return flag;
}

void DataCollectorServer::stop(){
    if (!serverStarted) return;
    Logger::getInstance()->info(F("Stopping uploader server..."));
    server.end();
    serverStarted = false;
    stopFlag = false;
    Logger::getInstance()->info(F("Uploader server stopped."));
}

void DataCollectorServer::expose_list_samplings_endpoint(){
    server.on("/samplings/list", HTTP_GET, [this](AsyncWebServerRequest *request){
        Logger::getInstance()->info(F("Received request to /list"));
        std::vector<String> files = sd->listFilesInDir("/samplings");
        String json = "{\"files\":[";
        for (size_t i = 0; i < files.size(); ++i) {
          json += "\"" + String(files[i]) + "\"";
          if (i < files.size() - 1) json += ",";
        }
        json += "]}";
        request->send(200, "application/json", json);
        Logger::getInstance()->debug("Responded with file list JSON: " + json);
      });
}

void DataCollectorServer::expose_download_endpoint(){
    server.on("/download", HTTP_GET, [this](AsyncWebServerRequest *request){
        Logger::getInstance()->info("Received request to /download");
  
        if (!request->hasParam("file")) {
          Logger::getInstance()->debug(F("Missing 'file' parameter in download request"));
          request->send(400, "application/json", "{\"error\":\"Missing 'file' parameter\"}");
          return;
        }
  
        String filepath = request->getParam("file")->value();
        Logger::getInstance()->debug("Requested file path: " + filepath);
  
        if (!sd->exists(filepath)) {
          Logger::getInstance()->debug("File not found: " + filepath);
          request->send(404, "application/json", "{\"error\":\"File not found\"}");
          return;
        }
  
        File file = sd->open(filepath);
        if (!file || file.isDirectory()) {
          request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
          return;
        }
  
        size_t fileSize = file.size();
        AsyncWebServerResponse *response = request->beginChunkedResponse("application/octet-stream",
          [file, this, filepath](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t {
            static MD5Builder md5;
            static bool md5Started = false;
            static size_t totalRead = 0;
  
            if (!md5Started) {
              md5.begin();
              md5Started = true;
            }
  
            size_t bytesRead = file.read(buffer, maxLen);
            if (bytesRead > 0) {
              md5.add(buffer, bytesRead);
              totalRead += bytesRead;
            }
  
            if (bytesRead == 0) {
              file.close();
              md5.calculate();
              String finalMd5 = md5.toString();
              md5Cache[filepath] = finalMd5;
              Logger::getInstance()->debug("MD5 calculated and cached: " + finalMd5);
              md5Started = false;
              totalRead = 0;
            }
  
            return bytesRead;
          });
  
        response->addHeader("Connection", "close");
        request->send(response);
        Logger::getInstance()->info("Streaming file: " + filepath);
      });
}

void DataCollectorServer::expose_validate_download_endpoint(){
    server.on("/validate", HTTP_GET, [this](AsyncWebServerRequest *request){
        Logger::getInstance()->info(F("Received request to /validate"));
  
        if (!request->hasParam("file") || !request->hasParam("md5")) {
          Logger::getInstance()->debug(F("Missing parameters in validate request"));
          request->send(400, "application/json", "{\"error\":\"Missing 'file' or 'md5' parameter\"}");
          return;
        }
  
        String filename = request->getParam("file")->value();
        String expected_md5 = request->getParam("md5")->value();
  
        Logger::getInstance()->debug(String("Validating file: ") + filename + " against MD5: " + expected_md5);
  
        if (md5Cache.find(filename) == md5Cache.end()) {
          Logger::getInstance()->debug(String("MD5 not found in cache for file: ") + filename);
          request->send(404, "application/json", "{\"error\":\"MD5 not cached\"}");
          return;
        }
  
        String actual_md5 = md5Cache[filename];
        Logger::getInstance()->debug("Cached MD5: " + actual_md5);
  
        if (actual_md5 == expected_md5) {
          Logger::getInstance()->debug(F("MD5 match confirmed."));
          request->send(200, "application/json", "{\"status\":\"MD5 match\"}");
        } else {
          Logger::getInstance()->debug(F("MD5 mismatch."));
          request->send(409, "application/json", "{\"error\":\"MD5 mismatch\"}");
        }
      });
}

void DataCollectorServer::expose_remove_all_samplings_endpoint(){
    server.on("/samplings/delete", HTTP_POST, [this](AsyncWebServerRequest *request){
        Logger::getInstance()->info(F("Received request to delete all files in /samplings"));
        bool success = sd->deleteAllFilesInDir("/samplings");
        if (success) {
          md5Cache.clear();
          Logger::getInstance()->debug(F("All files deleted successfully."));
          request->send(200, "application/json", "{\"status\":\"All files deleted\"}");
        } else {
          Logger::getInstance()->error(F("Failed to delete all files."));
          request->send(500, "application/json", "{\"error\":\"Failed to delete files\"}");
        }
      });
}

void DataCollectorServer::expose_ping_endpoint() {
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(204); // No Content
  });
}

void DataCollectorServer::expose_server_stop_endpoint(){
  server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request){
    DataCollectorServer::stopFlag = true;
    request->send(204); // No Content
  });
}
