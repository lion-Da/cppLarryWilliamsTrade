cmake_minimum_required(VERSION 3.10)
project(LarryWilliamsVolatilityBreakout VERSION 1.0)
# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++20 -DCURL_STATICLIB")

set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W1 /WX-")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W1 /WX-")
# Include directories
include_directories(include)
set(EXTERNAL_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../external)

# Source files, use to find all the files on sources
file(GLOB SOURCES "src/*.cpp")

# Create main executable
add_executable(volatility_breakout ${SOURCES})


# Find OpenSSL needed for the HMAC-SHA256
include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_LIBRARIES})
link_libraries(libssl libcrypto Crypt32)
# Find required packages
include_directories("${EXTERNAL_SOURCE_DIR}/curl-7.82.0/include")
link_directories("${EXTERNAL_SOURCE_DIR}/curl/lib/Release/")
link_libraries(libcurl)



# Find JSON library (nlohmann/json)
include_directories("${EXTERNAL_SOURCE_DIR}/json-3.12.0/include")
# Find libwebsockets

include_directories("${EXTERNAL_SOURCE_DIR}/libwebsockets/include")
include_directories("${EXTERNAL_SOURCE_DIR}/libwebsockets")

link_directories("${EXTERNAL_SOURCE_DIR}/libwebsockets/lib")

# Install target
install(TARGETS volatility_breakout DESTINATION bin)
