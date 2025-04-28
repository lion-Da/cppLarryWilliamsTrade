cmake_minimum_required(VERSION 3.10)
project(LarryWilliamsVolatilityBreakout VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create main executable
add_executable(volatility_breakout ${SOURCES})

# Find required packages
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})
target_link_libraries(volatility_breakout ${CURL_LIBRARIES})
# Find OpenSSL needed for the HMAC-SHA256 
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(volatility_breakout ${OPENSSL_LIBRARIES})
# Find JSON library (nlohmann/json)
find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(volatility_breakout nlohmann_json::nlohmann_json)

# Install target
install(TARGETS volatility_breakout DESTINATION bin)