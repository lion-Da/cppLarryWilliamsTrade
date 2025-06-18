#ifndef EXCHANGE_H
#define EXCHANGE_H
#include <string>
#include <vector>
#include <map>
#include <memory> 
#include "data_types.h"
using namespace std;

struct CommonFormatData
{
    std::string exchange;
    std::string symbol;
    std::vector<std::string> bids;
    std::vector<std::string> asks;
    int64_t timestamp; 
};

struct OrderBookData
{
    std::string exchange;
    std::string symbol;
    float best_bid;
    float best_ask;
    float bid_size;
    float ask_size;
    int64_t timestamp; // Timestamp in milliseconds
};



// 'small', 'medium', 'large'
#define RBITRAGE_STRATEGY "medium"

#define ORDERBOOK_DEPTH 5

// 套利参数 (基础配置，会被深度策略覆盖)
#define MIN_PROFIT_PERCENTAGE  0.0001  
#define MAX_POSITION_SIZE  1.0         

// # 风险控制
#define MAX_PRICE_DEVIATION  0.001     
#define PRICE_UPDATE_TIMEOUT  5       

class Exchange
{
    public:
        virtual ~Exchange() = default;
        virtual bool initialize(const string& api_key, const string& api_secret)  = 0;
        virtual vector<OHLCV> fetchHistoricalData(const string& symbol, const string& timeframe, const string& start_time, const string& end_time) = 0;
        virtual double getCurrentPrice(const string& symbol) = 0;
        virtual bool placeBuyOrder(const string& symbol, double quantity, double price = 0) = 0;
        virtual bool placeSellOrder(const string& symbol, double quantity, double price =0) = 0 ; 
        virtual vector<Order> getOpenOrders(const string& symbol) = 0;
        string getName() const { return name; }
        bool isConnected() const { return connected; }
    protected:
        string name;
        bool connected = false;
        string api_key;
        string api_secret;
        virtual string buildApiUrl(const string& endpoint) = 0;
        virtual string signRequest(const string& data) = 0;
};




#endif