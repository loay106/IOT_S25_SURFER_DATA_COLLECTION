#ifndef WIFI_HANDLER_H 
#define WIFI_HANDLER_H

#include <Arduino.h>
#include "../../Utils/Adresses.h"
#include <WiFi.h>
#include "../../Utils/Exceptions.h"
#include "Logger.h"


using namespace std;

class WifiHandler{
    private:
        const string ssid;
        const string password;
    public:
        static const int CONNECTION_TIMEOUT_MILLIS = 5000;
        WifiHandler(string ssid, string password): ssid(ssid), password(password){};

        void init(){
            WiFi.mode(WIFI_STA);
        };

        String getUnitMac(){
            return WiFi.macAddress();
        }

        void connect(){
            WiFi.mode(WIFI_STA);
            if(isWifiConnected()){
                return;
            }else{
                unsigned long startTime = millis();
                WiFi.begin(ssid.c_str(), password.c_str());
                while(!isWifiConnected() && (millis() - startTime) < CONNECTION_TIMEOUT_MILLIS){
                    delay(200);
                }
                if(!isWifiConnected()){
                    throw WifiError();
                }
            }            
        }
        void disconnect(){
            WiFi.disconnect(true);
            delay(100); // wait for wifi to disconnect fully
            WiFi.mode(WIFI_STA);
        }

        String getMacAddress() {
            uint8_t mac[6];
            WiFi.macAddress(mac);
            return macToString(mac).c_str();
        }

        int getChannel() {
            if(isWifiConnected()) {
                return WiFi.channel();
            }else{
                throw WifiError();
            }
        }

        bool isWifiConnected(){
            return WiFi.status() == WL_CONNECTED;
        }
};


#endif /* WIFI_HANDLER_H */
