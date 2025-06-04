#ifndef POSITION_H
#define POSITION_H
#include <string> 
#include <ctime>

class Position
{
public:
    Position(const std::string& symbol, double entryPrice, double quantity);

    void updatePrice(double currentPrice); 
    
    double calculatePnL() const; 
    double calculatePnLPercent() const;

    std::string getSymbol() const {return symbol;} 
    double getEntryPrice() const {return entryPrice;} 
    double getCurrentPrice() const { return currentPrice;} 
    double getQuantity() const { return quantity;} 
    std::time_t getEntryTime() const { return entryTime; }

    bool isLong() const {return quantity > 0;}  
private:
    std::string symbol;
    double entryPrice;
    double currentPrice;
    double quantity;
    std::time_t entryTime; 
};

#endif