#ifndef BINANCE_EXCHANGE_H
#define BINANCE_EXCHANGE_H

#include "exchange.h"
#include <string>
#include <curl/curl.h> 
class BinanceExchange : public Exchange
{
public: 
    BinanceExchange(); 
    ~BinanceExchange() override; 
    bool initialize(const std::string& api_key, const std::string& api_secret) override;
    std::vector<OHLCV> fetchHistoricalData(
        const std::string& symbol, 
        const std::string& timeframe,
        const std::string& start_time,
        const std::string& end_time 
    ) override;
    double getCurrentPrice(const std::string& symbol) override;
    bool placeBuyOrder(const std::string& symbol, double quantity, double price = 0) override;
    bool placeSellOrder(const std::string& symbol, double quantity, double price = 0) override;
    std::vector<Order> getOpenOrders(const std::string& symbol) override;
private:
    CURL * curl;
    std::string buildApiUrl(const std::string& endpoint) override;
    std::string signRequest(const std::string& date) override;
    static size_t WriteCallBack(void* contents, size_t size, size_t nmemb, std::string* s);
    std::string makeRequest(const std::string& url, const std::string& mehotd = "GET", 
    const std::string& data = "");
};
#endif