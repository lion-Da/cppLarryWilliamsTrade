#ifndef DATA_TYPES_H    
#define DATA_TYPES_H
#include <string> 
#include <vector>
#include<ctime> 
struct OHLCV{
    std::time_t timestamp;
    double open; 
    double high;
    double low;
    double close;
    double volume;
    std::string symbol; // Optional: to identify the trading pair
    double calculateRange() const 
    {
        return high - low; 
    }
};
enum class OrderType{
    MARKET,
    LIMIT
};
enum class OrderSide{
    BUY,
    SELL 
};
struct Order{
    std::string symbol;
    std::string orderId;
    OrderType type;
    OrderSide side;
    double quantity;
    double price;
    std::time_t timestamp;
    std::string status; // e.g., "pending", "filled", "canceled"
};
#endif
