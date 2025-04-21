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
        //simple exit strategy: close after 5 bars 
        if(!activeTrades.empty() && i - activeTrades[0].entryTime > 5)
        {
            Trade& trade = activeTrades[0];
            trade.exitPrice = current.close;
            trade.exitTime = current.timestamp;
            //Calculate profit 
            double profit = (trade.side == OrderSide::BUY) 
            ? (trade.exitPrice - trade.entryPrice) * trade.quantity 
            : (trade.entryPrice - trade.exitPrice) * trade.quantity;
            //Apply commision 
            double commission = trade.exitPrice * trade.quantity * commissionRate; 
            profit -= commission;

            trade.profit = profit; 
            trade.profitPercent = (profit / (trade.entryPrice * trade.quantity)) * 100.0;

            currentBalance += (trade.exitPrice * trade.quantity)  + profit; 
            if (trade.profit > 0) 
            {
                result.winningTrades++; 
            }
            else
            {
                result.losingTrades++;
            }
            std::cout << "Closing trade at " << trade.exitPrice <<" , Profit: " << trade.profit <<" (" << trade.profitPercent << std::endl;
            activeTrades.clear();
        }
    }
    if(!activeTrades.empty())
    {
        Trade& trade = activeTrades[0]; 
        trade.exitPrice = data.back().close;
        trade.exitTime = data.back().timestamp;
        double profit = (trade.side == OrderSide::BUY) 
        ? (trade.exitPrice - trade.entryPrice) * trade.quantity 
        : (trade.entryPrice - trade.exitPrice) * trade.quantity; 
        // Apply commission 
        double commission = trade.exitPrice * trade.quantity * commissionRate;
        profit -= commission; 

        trade.profit = profit;
        trade.profitPercent = (profit / (trade.entryPrice * trade.quantity)) * 100.0;

        //update 
        currentBalance += (trade.exitPrice * trade.quantity) + profit;
        // Track win/loss 
        if(profit > 0)
        {
            result.winningTrades++;
        }
        else
        {
            result.losingTrades++;
        }
        std::cout << "Closing final trade at " << trade.exitPrice << ", Profit: "
        << profit << " (" << trade.profitPercent << "%)" << std::endl;
        activeTrades.clear();
    }
    result.finalBalance = currentBalance;
    result.totalReturn = ((currentBalance - initialCapital) / initialCapital) * 100.0;
    result.maxDrawdown = calculateMaxDrawDown(result.equityCurve);
    result.winRate = (result.totalTrades > 0) ? static_cast<double>
}