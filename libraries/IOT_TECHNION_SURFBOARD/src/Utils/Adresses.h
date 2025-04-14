#ifndef UTILS_ADDRESSES_H 
#define UTILS_ADDRESSES_H

using namespace std;
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <vector>
#include <stdexcept>

inline string macToString(uint8_t mac[6]) {
    std::ostringstream macStream;
    for (int i = 0; i < 6; i++) {
        if (i > 0) {
            macStream << ":";
        }
        macStream << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
    }
    return macStream.str();
}

inline void stringToMac(const std::string &macString, uint8_t mac[6]) {
    std::istringstream macStream(macString);
    std::string byteStr;
    int i = 0;

    while (std::getline(macStream, byteStr, ':')) {
        if (i >= 6) {
            throw std::invalid_argument("Invalid MAC address format: too many bytes");
        }
        mac[i++] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
    }

    if (i != 6) {
        throw std::invalid_argument("Invalid MAC address format: not enough bytes");
    }
}

#endif /* UTILS_ADDRESSES_H */