#ifndef UTILS_ADDRESSES_H 
#define UTILS_ADDRESSES_H

#include <Arduino.h>
#include <stdint.h>
#include "Exceptions.h"

inline String macToString(uint8_t mac[6]) {
    String macStr;
    for (int i = 0; i < 6; i++) {
        if (i > 0) {
            macStr += "_";
        }
        if (mac[i] < 0x10) {
            macStr += "0"; // pad with leading zero
        }
        macStr += String(mac[i], HEX);
    }
    macStr.toUpperCase();
    return macStr;
}


inline void stringToMac(const String& macString, uint8_t mac[6]) {
    int start = 0;
    int end = macString.indexOf('_');
    int i = 0;

    while (end != -1) {
        if (i >= 6) {
            throw InvalidData();
        }
        String byteStr = macString.substring(start, end);
        mac[i++] = (uint8_t) strtoul(byteStr.c_str(), nullptr, 16);
        start = end + 1;
        end = macString.indexOf('_', start);
    }

    // Handle last byte
    if (i < 6) {
        String byteStr = macString.substring(start);
        mac[i++] = (uint8_t) strtoul(byteStr.c_str(), nullptr, 16);
    }

    if (i != 6) {
        throw InvalidData();
    }
}

inline String getHostname(String macAddress, bool isMain){
    String role;
    if(isMain){
      role = "main";
    }else{
      role = "sampler";
    }
    return "esp32-data-collector-" + role + "-" + macAddress;
}

#endif /* UTILS_ADDRESSES_H */