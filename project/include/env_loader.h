#ifndef ENV_LOADER_H
#define ENV_LOADER_H

#include <string>
#include <map>

class EnvLoader {
public:
    // Initialize and load environment variables
    static bool loadEnv(const std::string& filePath = ".env");
    
    // Get an environment variable
    static std::string get(const std::string& key, const std::string& defaultValue = "");
    
    // Check if an environment variable exists
    static bool has(const std::string& key);

private:
    static std::map<std::string, std::string> envVariables;
};

#endif // ENV_LOADER_H