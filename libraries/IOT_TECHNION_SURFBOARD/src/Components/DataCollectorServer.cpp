#include "DataCollectorServer.h"
#include "../Utils/Adresses.h"

DataCollectorServer::DataCollectorServer(SDCardHandler *sdHandler, const String &macAddress, bool isMain) : server(80), sd(sdHandler) {
    hostname = getHostname(macAddress, isMain);
    stopFlag = false;
}

void DataCollectorServer::begin()
{
    if (serverStarted) return;

    Logger::getInstance()->info("Starting uploader server...");

    if (!MDNS.begin(hostname.c_str())) {
      Logger::getInstance()->error("Error setting up mDNS responder");
      return;
    } else {
      Logger::getInstance()->info("mDNS responder started with hostname: " + std::string((hostname + ".local").c_str()));
      MDNS.addService("_http", "_tcp", 80);
      MDNS.addService("_surferdata", "_tcp", 80);
      MDNS.addServiceTxt("_http", "_tcp", "device", hostname);
      MDNS.addServiceTxt("_surferdata", "_tcp", "device", hostname);
      Logger::getInstance()->debug("mDNS services registered.");
    }

    expose_list_samplings_endpoint();
    expose_download_endpoint();
    expose_validate_download_endpoint();
    expose_remove_all_samplings_endpoint();
    expose_ping_endpoint();
    expose_server_stop_endpoint();

    server.begin();
    serverStarted = true;
    Logger::getInstance()->info("Uploader server started on port 80");
}

void DataCollectorServer::loop(){
    if (!serverStarted && WiFi.status() == WL_CONNECTED) {
        if (millis() - lastRetryTime >= retryInterval) {
          lastRetryTime = millis();
          Logger::getInstance()->info("Retrying uploader server startup...");
          begin();
        }
    }
}

bool DataCollectorServer::stopRequestReceived(){
    bool flag = stopFlag;
    return flag;
}

void DataCollectorServer::stop(){
    if (!serverStarted) return;
    Logger::getInstance()->info("Stopping uploader server...");
    server.end();
    serverStarted = false;
    stopFlag = false;
    Logger::getInstance()->info("Uploader server stopped.");
}

void DataCollectorServer::expose_list_samplings_endpoint(){
    server.on("/samplings/list", HTTP_GET, [this](AsyncWebServerRequest *request){
        Logger::getInstance()->info("Received request to /list");
        std::vector<std::string> files = sd->listFilesInDir("/samplings");
        String json = "{\"files\":[";
        for (size_t i = 0; i < files.size(); ++i) {
          json += "\"" + String(files[i].c_str()) + "\"";
          if (i < files.size() - 1) json += ",";
        }
        json += "]}";
        request->send(200, "application/json", json);
        Logger::getInstance()->debug("Responded with file list JSON: " + std::string(json.c_str()));
      });
}

void DataCollectorServer::expose_download_endpoint(){
    server.on("/download", HTTP_GET, [this](AsyncWebServerRequest *request){
        Logger::getInstance()->info("Received request to /download");
  
        if (!request->hasParam("file")) {
          Logger::getInstance()->debug("Missing 'file' parameter in download request");
          request->send(400, "application/json", "{\"error\":\"Missing 'file' parameter\"}");
          return;
        }
  
        String filepath = request->getParam("file")->value();
        Logger::getInstance()->debug("Requested file path: " + std::string(filepath.c_str()));
  
        if (!sd->exists(filepath)) {
          Logger::getInstance()->debug("File not found: " + std::string(filepath.c_str()));
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
              md5Cache[std::string(filepath.c_str())] = finalMd5;
              Logger::getInstance()->debug("MD5 calculated and cached: " + std::string(finalMd5.c_str()));
              md5Started = false;
              totalRead = 0;
            }
  
            return bytesRead;
          });
  
        response->addHeader("Connection", "close");
        request->send(response);
        Logger::getInstance()->info("Streaming file: " + std::string(filepath.c_str()));
      });
}

void DataCollectorServer::expose_validate_download_endpoint(){
    server.on("/validate", HTTP_GET, [this](AsyncWebServerRequest *request){
        Logger::getInstance()->info("Received request to /validate");
  
        if (!request->hasParam("file") || !request->hasParam("md5")) {
          Logger::getInstance()->debug("Missing parameters in validate request");
          request->send(400, "application/json", "{\"error\":\"Missing 'file' or 'md5' parameter\"}");
          return;
        }
  
        String filename = request->getParam("file")->value();
        String expected_md5 = request->getParam("md5")->value();
        std::string filename_key = std::string(filename.c_str());
  
        Logger::getInstance()->debug("Validating file: " + filename_key + " against MD5: " + std::string(expected_md5.c_str()));
  
        if (md5Cache.find(filename_key) == md5Cache.end()) {
          Logger::getInstance()->debug("MD5 not found in cache for file: " + filename_key);
          request->send(404, "application/json", "{\"error\":\"MD5 not cached\"}");
          return;
        }
  
        String actual_md5 = md5Cache[filename_key];
        Logger::getInstance()->debug("Cached MD5: " + std::string(actual_md5.c_str()));
  
        if (actual_md5 == expected_md5) {
          Logger::getInstance()->debug("MD5 match confirmed.");
          request->send(200, "application/json", "{\"status\":\"MD5 match\"}");
        } else {
          Logger::getInstance()->debug("MD5 mismatch.");
          request->send(409, "application/json", "{\"error\":\"MD5 mismatch\"}");
        }
      });
}

void DataCollectorServer::expose_remove_all_samplings_endpoint(){
    server.on("/samplings/delete", HTTP_POST, [this](AsyncWebServerRequest *request){
        Logger::getInstance()->info("Received request to delete all files in /samplings");
        bool success = sd->deleteAllFilesInDir("/samplings");
        if (success) {
          md5Cache.clear();
          Logger::getInstance()->debug("All files deleted successfully.");
          request->send(200, "application/json", "{\"status\":\"All files deleted\"}");
        } else {
          Logger::getInstance()->error("Failed to delete all files.");
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
    stopFlag = true;
    request->send(204); // No Content
});
}
