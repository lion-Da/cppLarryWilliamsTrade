#ifndef BACKTEST_ENGINE_H
#define BACKTEST_ENGINE_H
#include <memory> 
#include <vector> 
#include <string> 
#include "strategy.h" 
#include "position.h" 
#include "data_types.h"
struct BacktestResult{
    double initialBalance;
    double finalBalance;
    double totalReturn;
    double maxDrawdown;
    int totalTrades; 
    int winningTrades; 
    int losingTrades; 
    double winRate;
    std::vector<double> equityCurve;
};
struct Trade
{
    std::string symbol; 
    OrderSide side;
    double entryPrice;
    double exitPrice;
    double quantity;
    std::time_t entryTime;
    std::time_t exitTime;
    double profit; 
    double profitPercent; 
};

class BacktestEngine
{
    BacktestEngine();

    void setInitialCapital(double capital);
    void setCommissionRate(double rate);

    BacktestResult runBacktest(
        std::shared_ptr<Strategy> strategy,
        const std::vector<OHLCV>& data,
        double initialCapital = 10000.0
    );
    std::string generateReport(const BacktestResult & result) const;
private:
    double initialCapital = 10000.0;
    double commissionRate = 0.001;

    double calculateMaxDrawDown(const std::vector<double>& equityCurve) const;
};
#endif 