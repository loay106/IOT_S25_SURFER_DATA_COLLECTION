#ifndef UTILS_ADDRESSES_H 
#define UTILS_ADDRESSES_H

using namespace std;
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <vector>
#include <stdexcept>

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


inline void stringToMac(const std::string &macString, uint8_t mac[6]) {
    std::istringstream macStream(macString);
    std::string byteStr;
    int i = 0;

    while (std::getline(macStream, byteStr, '_')) {
        if (i >= 6) {
            throw std::invalid_argument("Invalid MAC address format: too many bytes");
        }
        mac[i++] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
    }

    if (i != 6) {
        throw std::invalid_argument("Invalid MAC address format: not enough bytes");
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