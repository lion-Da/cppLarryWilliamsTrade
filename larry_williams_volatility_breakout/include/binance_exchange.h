#ifndef BINANCE_EXCHANGE_H
#define BINANCE_EXCHANGE_H

#include "exchange.h"
#include <string>
#include <curl/curl.h>
#include <memory>
#include <functional>
#include "data_types.h"
#include "websocket_client.h"
// Forward declaration
class WebSocketClient;

class BinanceExchange : public Exchange {
public:
    BinanceExchange();
    ~BinanceExchange() override;
    
    // Implementation of Exchange interface
    bool initialize(const std::string& api_key, const std::string& api_secret) override;
    std::vector<OHLCV> fetchHistoricalData(const std::string& symbol, 
                                          const std::string& timeframe,
                                          const std::string& start_time,
                                          const std::string& end_time) override;
    double getCurrentPrice(const std::string& symbol) override;
    
    bool placeBuyOrder(const std::string& symbol, double quantity, double price = 0) override;
    bool placeSellOrder(const std::string& symbol, double quantity, double price = 0) override;
    std::vector<Order> getOpenOrders(const std::string& symbol) override;
    
    // WebSocket methods
    bool connectWebSocket(const std::string& symbol, const std::string& channel = "ticker");
    void disconnectWebSocket();
    bool isWebSocketConnected() const;
    
    // Set callbacks for real-time data
    void setRealTimePriceCallback(std::function<void(const std::string&, double)> callback);
    void setRealTimeCandleCallback(std::function<void(const OHLCV&)> callback);
    
    // WebSocket message handler
    void handleWebSocketMessage(const std::string& message);
    
private:
    CURL* curl;
    
    // Implementation of helper methods
    std::string buildApiUrl(const std::string& endpoint) override;
    std::string signRequest(const std::string& data) override;
    
    // Helper functions for API requests
    static size_t WriteCallBack(void* contents, size_t size, size_t nmemb, std::string* s);
    std::string makeRequest(const std::string& url, const std::string& method = "GET", 
                          const std::string& data = "");

    // Get server time for timestamp
    long long getServerTime();
    
    // Helper to format HMAC-SHA256 signature for Binance
    std::string createSignature(const std::string& data);
    
    // WebSocket support
    std::unique_ptr<WebSocketClient> websocket;
    std::function<void(const std::string&, double)> priceUpdateCallback;
    std::function<void(const OHLCV&)> candleUpdateCallback;
};

#endif // BINANCE_EXCHANGE_H