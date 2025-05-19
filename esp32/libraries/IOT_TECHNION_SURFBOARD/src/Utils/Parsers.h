#ifndef PARSERS_H 
#define PARSERS_H

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

inline std::vector<std::string> parseSensorParams(const std::string& input) {
    // Ensure the input starts with '[' and ends with ']'
    if (input.empty() || input.front() != '[' || input.back() != ']') {
        throw std::invalid_argument("Invalid format: Input must start with '[' and end with ']'.");
    }

    // Extract the content inside the brackets
    std::string content = input.substr(1, input.size() - 2);

    std::vector<std::string> result;
    std::string currentParam;

    for (char c : content) {
        if (c == ',') {
            // Push the current parameter to the vector
            result.push_back(currentParam);
            currentParam.clear();
        } else {
            currentParam += c;
        }
    }

    // Push the last parameter if there is any
    if (!currentParam.empty()) {
        result.push_back(currentParam);
    }

    return result;
}


#endif /* PARSERS_H */