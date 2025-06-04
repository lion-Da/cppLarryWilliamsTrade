#ifndef STRATEGY_H
#define STRATEGY_H

#include <string> 
#include <vector>
#include <memory>
#include <map>
#include "exchange.h"
#include "data_types.h"
using namespace std;
struct Signal{
    string symbol;
    OrderSide side;
    double suggestedPrice;
    double suggestedQuantity;
    time_t timestamp;
    string reason; 
};

class Strategy{
public:
    virtual ~Strategy() = default;
    virtual bool initialize(const map<string, double>& parameters) = 0;
    virtual vector<Signal> processData(const vector<OHLCV>& data) = 0;
    virtual string getName() const = 0; 
    void setExchange(std::shared_ptr<Exchange> exch) {
        exchange = exch;
    }
    // Get current parameters
    std::map<std::string, double> getParameters() const {
        return parameters;
    }
protected:
    std::map<std::string, double> parameters;
    std::shared_ptr<Exchange> exchange;
};


#endif