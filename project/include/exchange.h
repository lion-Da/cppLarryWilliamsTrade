#ifndef EXCHANGE_H
#define EXCHANGE_H
#include <string>
#include <vector>
#include <map>
#include <memory> 
#include "data_types.h"
using string = std::string;
class Exchange
{

    public:
        virtual ~Exchange() = default;
        virtual bool initialize(const string& api_key, const string& api_secret)  = 0;
        virtual std::vector<OHLCV> fetchHistoricalData(const string& symbol, const string& timeframe, const string& start_time, const string& end_time) = 0;
        virtual double getCurrentPrice(const string& symbol) = 0;
        virtual bool placeBuyOrder(const string& symbol, double quantity, double price = 0) = 0;
        virtual bool placeSellOrder(const string& symbol, double quantity, double price =0) = 0 ; 
        virtual std::vector<Order> getOpenOrders(const string& symbol) = 0;
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