#include "websocket_client.h"
#include <libwebsockets.h>
#include <iostream>
#include <cstring>
#include <algorithm>

// Structure to hold per-session data
struct PerSessionData {
    WebSocketClient* client;
    std::string receivedMessage;
};

// Protocols we implement
const struct lws_protocols WebSocketClient::protocols[] = {
    {
        "trading-protocol",        // Protocol name
        WebSocketClient::callbackFunction,  // Callback (private but accessible)
        sizeof(PerSessionData),    // Per-session data size
        0,                         // Rx buffer size (0 = default)
    },
    { NULL, NULL, 0, 0 }  // End of list
};

WebSocketClient::WebSocketClient() 
    : context(nullptr), connection(nullptr), running(false), connected(false), sessionData(nullptr) {
}

WebSocketClient::~WebSocketClient() {
    disconnect();
    
    if (context) {
        lws_context_destroy(context);
        context = nullptr;
    }
}

bool WebSocketClient::initialize() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    
    info.port = CONTEXT_PORT_NO_LISTEN;  // Client-only
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    
    // Initialize libwebsockets logs
    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);
    
    context = lws_create_context(&info);
    
    if (!context) {
        if (errorCallback) {
            errorCallback("Failed to create WebSocket context");
        }
        return false;
    }
    
    return true;
}

bool WebSocketClient::connect(const std::string& url) {
    if (!context) {
        if (errorCallback) {
            errorCallback("WebSocket context not initialized");
        }
        return false;
    }
    
    std::cout << "Connecting to WebSocket: " << url << std::endl;
    
    // Parse URL components
    std::string protocol, host, path, port;
    bool secure = false;
    
    if (url.substr(0, 6) == "wss://") {
        protocol = "wss";
        host = url.substr(6);
        secure = true;
        port = "443";
    } else if (url.substr(0, 5) == "ws://") {
        protocol = "ws";
        host = url.substr(5);
        secure = false;
        port = "80";
    } else {
        if (errorCallback) {
            errorCallback("Invalid WebSocket URL: " + url);
        }
        return false;
    }
    
    // Extract path
    size_t pathPos = host.find_first_of('/');
    if (pathPos != std::string::npos) {
        path = host.substr(pathPos);
        host = host.substr(0, pathPos);
    } else {
        path = "/";
    }
    
    // Extract port if specified
    size_t portPos = host.find_first_of(':');
    if (portPos != std::string::npos) {
        port = host.substr(portPos + 1);
        host = host.substr(0, portPos);
    }
    std::cout << "Protocol: " << protocol << ", Host: " << host 
              << ", Path: " << path << ", Port: " << port << std::endl;
    // Setup connection info
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = host.c_str();
    ccinfo.port = std::stoi(port);
    ccinfo.path = path.c_str();
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = host.c_str();
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = secure ? LCCSCF_USE_SSL : 0;
    // User data to be accessible in the callback
    if (sessionData == nullptr) {
        sessionData = new PerSessionData();
        sessionData->client = this;
    }
    ccinfo.userdata = sessionData;
    // Initiate connection
    connection = lws_client_connect_via_info(&ccinfo);
    if (!connection) {
        if (errorCallback) {
            errorCallback("Failed to connect to: " + url);
        }
        return false;
    }
    // Start event loop in a separate thread
    running = true;
    eventThread = std::thread(&WebSocketClient::eventLoop, this);
    return true;
}

void WebSocketClient::disconnect() {
    // Signal the event loop to stop
    running = false;
    // Wait for the event thread to finish
    if (eventThread.joinable()) {
        eventThread.join();
    }
    // Clean up connection
    connection = nullptr;
    connected = false;
    
    // Clean up session data
    if (sessionData) {
        delete sessionData;
        sessionData = nullptr;
    }
    
    // Notify about disconnection
    if (connectionCallback) {
        connectionCallback(false);
    }
}

bool WebSocketClient::send(const std::string& message) {
    if (!connected || !connection) {
        if (errorCallback) {
            errorCallback("Cannot send message: not connected");
        }
        return false;
    }
    
    std::cout << "Sending WebSocket message: " << message << std::endl;
    
    // Add message to send queue
    {
        std::lock_guard<std::mutex> lock(sendMutex);
        sendQueue.push(message);
    }
    
    // Request a callback when the socket is writeable
    lws_callback_on_writable(connection);
    
    return true;
}

bool WebSocketClient::isConnected() const {
    return connected;
}

void WebSocketClient::setMessageCallback(MessageCallback callback) {
    messageCallback = callback;
}

void WebSocketClient::setConnectionCallback(ConnectionCallback callback) {
    connectionCallback = callback;
}

void WebSocketClient::setErrorCallback(ErrorCallback callback) {
    errorCallback = callback;
}

void WebSocketClient::onMessage(const std::string& message) {
    std::cout << "WebSocket received: " << message << std::endl;
    if (messageCallback) {
        messageCallback(message);
    }
}

bool WebSocketClient::getConnectionStatus() const {
    return connected;
}

void WebSocketClient::setConnectionStatus(bool status) {
    connected = status;
    if (connectionCallback) {
        connectionCallback(status);
    }
}

void WebSocketClient::eventLoop() {
    int n = 0;
    while (running && n >= 0) {
        n = lws_service(context, 50); // 50ms timeout
        // Small sleep to prevent high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int WebSocketClient::callbackFunction(struct lws* wsi, enum lws_callback_reasons reason,
                                     void* user, void* in, size_t len) {
    PerSessionData* data = static_cast<PerSessionData*>(user);
    WebSocketClient* client = nullptr;
    if (data) {
        client = data->client;
    }
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            // Connection established
            if (client) {
                std::cout << "WebSocket connection established" << std::endl;
                client->setConnectionStatus(true);
            }
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            // Connection error
            if (client) {
                client->setConnectionStatus(false);
                const char* msg = in ? static_cast<const char*>(in) : "Unknown connection error";
                std::cerr << "WebSocket connection error: " << (in ? msg : "Unknown error") << std::endl;
                
                if (client->errorCallback) {
                    client->errorCallback(in ? msg : "Unknown connection error");
                }
            }
            break;
            
        case LWS_CALLBACK_CLIENT_RECEIVE:
            // Data received
            if (client && in && len > 0) {
                // Append to buffer
                data->receivedMessage.append(static_cast<const char*>(in), len);
                
                // If this is the final fragment
                if (lws_is_final_fragment(wsi)) {
                    client->onMessage(data->receivedMessage);
                    data->receivedMessage.clear();
                }
            }
            break;
            
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            // Socket is writeable, send queued messages
            if (client) {
                std::string message;
                
                // Get a message from the queue
                {
                    std::lock_guard<std::mutex> lock(client->sendMutex);
                    if (!client->sendQueue.empty()) {
                        message = client->sendQueue.front();
                        client->sendQueue.pop();
                    }
                }
                
                if (!message.empty()) {
                    // LWS_PRE bytes of padding are needed
                    unsigned char* buf = new unsigned char[LWS_PRE + message.length()];
                    memcpy(&buf[LWS_PRE], message.c_str(), message.length());
                    
                    // Write the message
                    int result = lws_write(wsi, &buf[LWS_PRE], message.length(), LWS_WRITE_TEXT);
                    
                    delete[] buf;
                    
                    if (result < 0) {
                        if (client->errorCallback) {
                            client->errorCallback("Error writing to WebSocket");
                        }
                        return -1;
                    }
                    
                    // If more messages, request another callback
                    {
                        std::lock_guard<std::mutex> lock(client->sendMutex);
                        if (!client->sendQueue.empty()) {
                            lws_callback_on_writable(wsi);
                        }
                    }
                }
            }
            break;
            
        case LWS_CALLBACK_CLIENT_CLOSED:
            // Connection closed
            if (client) {
                std::cout << "WebSocket connection closed" << std::endl;
                client->setConnectionStatus(false);
            }
            break;
            
        default:
            break;
    }
    return 0;
}