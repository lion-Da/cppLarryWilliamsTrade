#include "env_loader.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <cstdlib>

// Initialize static member
std::map<std::string, std::string> EnvLoader::envVariables;

bool EnvLoader::loadEnv(const std::string& filePath) {
    std::cout << "Loaded env variable start" << std::endl;
    // Try to open the .env file
    std::ifstream envFile(filePath);
    if (!envFile.is_open()) {
        std::cerr << "Warning: Could not open .env file at " << filePath << std::endl;
        return false;
    }

    std::string line;
    std::regex envRegex("^([A-Za-z0-9_]+)=(.*)$");
    std::smatch match;

    // Read each line
    while (std::getline(envFile, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Match key-value pairs
        if (std::regex_search(line, match, envRegex)) {
            std::string key = match[1];
            std::string value = match[2];
            std::cout << "Loaded env variable: " << key << " = " << value << std::endl;
            // Remove quotes if present
            if (value.size() >= 2 && 
                ((value.front() == '"' && value.back() == '"') || 
                 (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2);
            }
            
            // Store in our map
            envVariables[key] = value;
            
            // Also set as actual environment variable (optional)
            #ifdef _WIN32
                _putenv_s(key.c_str(), value.c_str());
            #else
                setenv(key.c_str(), value.c_str(), 1);
            #endif
        }
    }

    envFile.close();
    return true;
}

std::string EnvLoader::get(const std::string& key, const std::string& defaultValue) {
    // First check our loaded variables
    auto it = envVariables.find(key);
    if (it != envVariables.end()) {
        return it->second;
    }
    
    // Then check system environment variables
    const char* envVar = std::getenv(key.c_str());
    if (envVar != nullptr) {
        return std::string(envVar);
    }
    
    // Return default if not found
    return defaultValue;
}

bool EnvLoader::has(const std::string& key) {
    return envVariables.find(key) != envVariables.end() || std::getenv(key.c_str()) != nullptr;
}