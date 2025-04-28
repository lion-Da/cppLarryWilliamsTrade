#ifndef OKX_EXCHANGE_H
#define OKX_EXCHANGE_H
#include "exchange.h"
#include <string>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <ctime>
class OKXExchange : public Exchange
{
    public:
        OKXExchange();
        ~OKXExchange() override;
        bool initialize(const std::string& api_key, const std::string& api_secret) override;
        std::vector<OHLCV> fetchHistoricalData(const std::string& symbol, 
                                              const std::string& timeframe,
                                              const std::string& start_time,
                                              const std::string& end_time) override;
        double getCurrentPrice(const std::string& symbol) override;
        
        bool placeBuyOrder(const std::string& symbol, double quantity, double price = 0) override;
        bool placeSellOrder(const std::string& symbol, double quantity, double price = 0) override;
        std::vector<Order> getOpenOrders(const std::string& symbol) override;
        
        // Set passphrase (OKX specific)
        void setPassphrase(const std::string& passphrase);
    private:
    CURL* curl;
    std::string passphrase; // OKX requires a passphrase in addition to key/secret
    
    // Implementation of helper methods
    std::string buildApiUrl(const std::string& endpoint) override;
    std::string signRequest(const std::string& data) override;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);
    std::string makeRequest(const std::string& url, const std::string& method = "GET", 
                            const std::string& data = "", const std::string& timestamp = "");
    
    // Helper to format symbol for OKX (e.g., BTC-USDT instead of BTCUSDT)
    std::string formatSymbol(const std::string& symbol);
    
    // HMAC-SHA256 signing function
    std::string generateHMAC(const std::string& key, const std::string& data);
    
    // Get timestamp in ISO8601 format for OKX API
    std::string getTimestamp();
};


#endif