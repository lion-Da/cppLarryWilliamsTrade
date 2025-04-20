#include "position.h" 
#include <cmath> 

Position::Position(const std::string& symbol, double entry, double qty)
:symbol(symbol), entryPrice(entry), quantity(qty), currentPrice(entry)
{
    entryTime = std::time(nullptr);  // set Entry time to current time 
}
void Position::updatePrice(double price)
{
    currentPrice = price;
}
double Position::calculatePnL() const 
{
    if (quantity > 0) // Long position
    {
        return (currentPrice - entryPrice) * quantity; 
    }
    else // Short position
    {
        return (entryPrice - currentPrice) * std::abs(quantity); 
    }
}

double Position::calculatePnLPercent() const 
{
    double pnl = calculatePnL();
    double investment = std::abs(quantity) * entryPrice;
    if(investment <= 0)
    {
        return 0.0;
    }
    return (pnl / investment) * 100.0;
}