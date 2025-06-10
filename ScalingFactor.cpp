#include "ScalingFactor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

bool ScalingFactor::loadFromFile(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;
    
    // Read the entire file into a string
    std::string jsonContent((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    
    // Very basic JSON parser for {key:value} pairs
    // This is a simple implementation and assumes well-formed JSON
    // In a production system, use a proper JSON library like nlohmann::json
    
    size_t pos = 0;
    while ((pos = jsonContent.find("\"", pos)) != std::string::npos) {
        // Find the sector name
        size_t keyStart = pos + 1;
        size_t keyEnd = jsonContent.find("\"", keyStart);
        if (keyEnd == std::string::npos) break;
        
        std::string sector = jsonContent.substr(keyStart, keyEnd - keyStart);
        
        // Find the value
        size_t valueStart = jsonContent.find(":", keyEnd);
        if (valueStart == std::string::npos) break;
        
        // Skip whitespace after colon
        valueStart++;
        while (valueStart < jsonContent.length() && 
               (jsonContent[valueStart] == ' ' || jsonContent[valueStart] == '\t')) {
            valueStart++;
        }
        
        // Find end of value (comma or closing brace)
        size_t valueEnd = jsonContent.find_first_of(",}", valueStart);
        if (valueEnd == std::string::npos) break;
        
        // Extract and parse the value
        std::string valueStr = jsonContent.substr(valueStart, valueEnd - valueStart);
        try {
            double factor = std::stod(valueStr);
            factors[sector] = factor;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing factor for sector " << sector << ": " << e.what() << std::endl;
        }
        
        pos = valueEnd;
    }
    
    return !factors.empty();
}

double ScalingFactor::getFactor(const std::string& sector) const {
    auto it = factors.find(sector);
    return (it != factors.end()) ? it->second : 1.0;
}
