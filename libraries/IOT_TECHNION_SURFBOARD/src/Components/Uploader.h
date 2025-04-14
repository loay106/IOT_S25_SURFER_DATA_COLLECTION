#ifndef UPLOADER_H 
#define UPLOADER_H

#include "../Components/IO/WifiHandler.h"
#include "../Components/IO/SDCardHandler.h"
#include "../Components/IO/Logger.h"

#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <MD5Builder.h>


class Uploader {
private:
  AsyncWebServer server;
  SDCardHandler& sd;
  bool serverStarted = false;
  unsigned long lastRetryTime = 0;
  const unsigned long retryInterval = 5000;
  String hostname;

  String calculateMD5(const String& filepath) {
    Logger::getInstance()->debug("Calculating MD5 for: " + filepath.c_str());
    File file = sd.open(filepath.c_str());
    if (!file) {
      Logger::getInstance()->error("Failed to open file for MD5: " + filepath.c_str());
      return "";
    }

    MD5Builder md5;
    md5.begin();
    uint8_t buf[512];
    while (file.available()) {
      size_t len = file.read(buf, sizeof(buf));
      md5.add(buf, len);
    }
    file.close();
    md5.calculate();
    String result = md5.toString();
    Logger::getInstance()->debug("MD5 result: " + result.c_str());
    return result;
  }

public:
  Uploader(SDCardHandler& sdHandler, const String& macAddress) : server(80), sd(sdHandler) {
    hostname = "esp32-" + macAddress;
  }

  void begin() {
    if (serverStarted) return;

    if (!WiFi.isConnected()) {
      Logger::getInstance()->error("WiFi not connected. Cannot start web server. Will retry...");
      return;
    }

    Logger::getInstance()->info("Starting uploader server...");

    if (!MDNS.begin(hostname.c_str())) {
      Logger::getInstance()->error("Error setting up mDNS responder");
    } else {
      Logger::getInstance()->info("mDNS responder started with hostname: " + hostname + ".local");
      MDNS.addService("_http", "_tcp", 80);
      MDNS.addService("_surferdata", "_tcp", 80);
      MDNS.addServiceTxt("_http", "_tcp", "device", hostname);
      MDNS.addServiceTxt("_surferdata", "_tcp", "device", hostname);
    }

    server.on("/list", HTTP_GET, [this](AsyncWebServerRequest *request){
      Logger::getInstance()->info("Received request to /list");
      String fileList = sd.listFiles("/samplings");
      String json = "{\"files\":[";
      fileList.trim();
      fileList.replace("\n", "\",\" ");
      json += fileList;
      if (json.endsWith(",")) json.remove(json.length() - 1);
      json += "]}";
      request->send(200, "application/json", json);
      Logger::getInstance()->debug("Listed files: \n" + fileList);
    });

    server.on("/download", HTTP_GET, [this](AsyncWebServerRequest *request){
      Logger::getInstance()->info("Received request to /download");

      if (!request->hasParam("file")) {
        request->send(400, "application/json", "{\"error\":\"Missing 'file' parameter\"}");
        return;
      }

      String filename = request->getParam("file")->value();
      String filepath = "/samplings/" + filename;
      Logger::getInstance()->debug("Requested file: " + filepath.c_str());

      if (!sd.exists(filepath)) {
        request->send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
      }

      request->send(sd.getFS(), filepath, "application/octet-stream", true);
      Logger::getInstance()->info("Serving file: " + filepath.c_str());
    });

    server.on("/validate", HTTP_GET, [this](AsyncWebServerRequest *request){
      Logger::getInstance()->info("Received request to /validate");

      if (!request->hasParam("file") || !request->hasParam("md5")) {
        request->send(400, "application/json", "{\"error\":\"Missing 'file' or 'md5' parameter\"}");
        return;
      }

      String filename = request->getParam("file")->value();
      String expected_md5 = request->getParam("md5")->value();
      String filepath = "/samplings/" + filename;

      Logger::getInstance()->debug("Validating file: " + filepath.c_str() + " with MD5: " + expected_md5.c_str());

      if (!sd.exists(filepath)) {
        request->send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
      }

      String actual_md5 = calculateMD5(filepath);
      if (actual_md5 == expected_md5) {
        request->send(200, "application/json", "{\"status\":\"MD5 match\"}");
      } else {
        request->send(409, "application/json", "{\"error\":\"MD5 mismatch\"}");
      }
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
      String status = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
      String json = "{\"status\":\"" + status + "\"}";
      request->send(200, "application/json", json);
    });

    server.begin();
    serverStarted = true;
    Logger::getInstance()->info("Uploader server started on port 80");
  }

  void update() {
    if (!serverStarted && WiFi.status() == WL_CONNECTED) {
      if (millis() - lastRetryTime >= retryInterval) {
        lastRetryTime = millis();
        Logger::getInstance()->info("Retrying uploader server startup...");
        begin();
      }
    }
  }

  void stop() {
    if (!serverStarted) return;
    Logger::getInstance()->info("Stopping uploader server...");
    server.end();
    serverStarted = false;
    Logger::getInstance()->info("Uploader server stopped.");
  }
};

#endif /* UPLOADER_H */