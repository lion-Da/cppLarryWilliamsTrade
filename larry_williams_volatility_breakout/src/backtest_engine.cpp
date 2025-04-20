#include "backtest_engine.h"
#include <iostream>
#include <algorithm>
#include <numeric> 
#include <cmath> 
#include <sstream> 
#include <iomanip> 

BacktestEngine::BacktestEngine() = default;

void BacktestEngine::setInitialCapital(double capital)
{
    initialCapital = capital;
}
void BacktestEngine::setCommissionRate(double rate)
{
    commissionRate = rate;
}
BacktestResult BacktestEngine::runBacktest(
    std::shared_ptr<Strategy> strategy,
    const std::vector<OHLCV>& data,
    double initialCapital
)
{
    this->initialCapital = initialCapital;
    BacktestResult result;
    result.totalTrades = 0;
    result.winningTrades = 0;
    result.losingTrades = 0;

    result.equityCurve.push_back(initialCapital);
    double currentBalance = initialCapital;
    std::vector<Trade> activeTrades;
    for (size_t i = 1; i < data.size(); ++i)
    {
        const OHLCV& current = data[i];
        result.equityCurve.push_back(currentBalance);
        std::vector<Signal> signals;
        std::vector<OHLCV> historicalData(data.begin(), data.begin() + i + 1);
        signals = strategy->processData(historicalData);
        for (const auto& signal: signals)
        {
            if(activeTrades.empty())
            {
                double positionSize = currentBalance * 0.02/current.close;
                Trade trade; 
                trade.symbol = signal.symbol;
                trade.side = signal.side; 
                trade.entryPrice = current.close;
                trade.entryTime = current.timestamp;
                trade.quantity = positionSize;
                double commission = trade.entryPrice * trade.quantity * commissionRate;
                currentBalance -= commission;
                activeTrades.push_back(trade);
                result.totalTrades++;
                std::cout << "Opening trade at " << trade.entryPrice << ", Quantity: " << trade.quantity << std::endl;
            }
        }
        if(!active)
    }
}