#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <cstdint>


int generate_request_id() {
    static int counter = 0;
    if (counter == INT32_MAX) {
        counter = 0;
    }
    return ++counter;
}


std::string get_free_port() {
    static int counter = 5555;
    if (counter >= 65535) {
        throw std::runtime_error("All ports is busy.");
    }
    return std::to_string(++counter);
}


std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}


std::string join(const std::vector<std::string>& elements, const std::string& delimiter) {
    std::ostringstream result;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i != 0) {
            result << delimiter;
        }
        result << elements[i];
    }
    return result.str();
}