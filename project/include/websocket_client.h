#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <memory>
#include "data_types.h"
#include "libwebsockets.h"

// Forward declarations
struct lws;
struct lws_context;
struct PerSessionData;

class WebSocketClient {
public:
    // Callback types
    using MessageCallback = std::function<void(const std::string&)>;
    using ConnectionCallback = std::function<void(bool)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    WebSocketClient();
    ~WebSocketClient();
    
    // Initialize the WebSocket client
    bool initialize();
    
    // Connect to a WebSocket endpoint
    bool connect(const std::string& url);
    
    // Disconnect from the server
    void disconnect();
    
    // Send a message to the server
    bool send(const std::string& message);
    
    // Check if connected
    bool isConnected() const;
    
    // Set callbacks
    void setMessageCallback(MessageCallback callback);
    void setConnectionCallback(ConnectionCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // Call the message callback (called by the static callback)
    void onMessage(const std::string& message);
    
    // Get connection status
    bool getConnectionStatus() const;
    void setConnectionStatus(bool status);
    static int callbackFunction(struct lws* wsi, enum lws_callback_reasons reason,
        void* user, void* in, size_t len);
    // Static callback for libwebsockets
    static struct lws_protocols protocols[];
private:
    // libwebsockets context and connection
    lws_context* context;
    lws* connection;
    
    // Thread for WebSocket event loop
    std::thread eventThread;
    std::atomic<bool> running;
    std::atomic<bool> connected;
    
    // Mutex for thread safety
    std::mutex mutex;
    
    // Callbacks
    MessageCallback messageCallback;
    ConnectionCallback connectionCallback;
    ErrorCallback errorCallback;
    
    // Message queue for sending
    std::queue<std::string> sendQueue;
    std::mutex sendMutex;
    
    // Event loop
    void eventLoop();
    
    // Pointer to the session data
    PerSessionData* sessionData;
};

#endif // WEBSOCKET_CLIENT_H