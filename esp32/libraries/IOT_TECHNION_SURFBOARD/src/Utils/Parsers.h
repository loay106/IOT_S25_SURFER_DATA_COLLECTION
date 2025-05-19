#ifndef PARSERS_H 
#define PARSERS_H

#include <Arduino.h>
#include <vector>
#include "Exceptions.h"
#include <iostream>

inline std::vector<String> parseSensorParams(const String& input) {
    if (input.length() < 2 || input.charAt(0) != '[' || input.charAt(input.length() - 1) != ']') {
        throw InvalidData();
    }

    String content = input.substring(1, input.length() - 1);
    std::vector<String> result;
    String currentParam;

    for (int i = 0; i < content.length(); i++) {
        char c = content.charAt(i);
        if (c == ',') {
            result.push_back(currentParam);
            currentParam = "";
        } else {
            currentParam += c;
        }
    }

    if (currentParam.length() > 0) {
        result.push_back(currentParam);
    }

    return result;
}



#endif /* PARSERS_H */