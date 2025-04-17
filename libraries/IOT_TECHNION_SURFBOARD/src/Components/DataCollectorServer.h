#ifndef DATA_COLLECTOR_SERVER_H 
#define DATA_COLLECTOR_SERVER_H

#include "../Components/IO/WifiHandler.h"
#include "../Components/IO/SDCardHandler.h"
#include "../Components/IO/Logger.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <MD5Builder.h>
#include <vector>
#include <map>


class DataCollectorServer {
  private:
    AsyncWebServer server;
    SDCardHandler* sd;
    bool serverStarted = false;
    unsigned long lastRetryTime = 0;
    const unsigned long retryInterval = 5000;
    String hostname;
    std::map<std::string, String> md5Cache;
    static bool stopFlag;

    void expose_list_samplings_endpoint();
    void expose_download_endpoint();
    void expose_validate_download_endpoint();
    void expose_remove_all_samplings_endpoint();
    void expose_ping_endpoint();
    void expose_server_stop_endpoint();

  public:
    DataCollectorServer(SDCardHandler* sdHandler, const String& macAddress, bool isMain);

    void begin();
    void loop();
    bool stopRequestReceived();
    void stop();
};

#endif /* DATA_COLLECTOR_SERVER_H */
