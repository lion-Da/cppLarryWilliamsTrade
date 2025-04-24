
#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <iomanip>
#include "data_types.h"
#include "exchange.h"
#include "binance_exchange.h"
#include "volatility_breakout.h"
#include "backtest_engine.h"

int main() {
    // Create exchange instance
    auto exchange = std::make_shared<BinanceExchange>();
    
    // Initialize exchange (with empty API keys for now)
    if (!exchange->initialize("", "")) {
        std::cerr << "Failed to initialize exchange" << std::endl;
        return 1;
    }
    
    // Create strategy instance
    auto strategy = std::make_shared<VolatilityBreakout>();
    
    // Configure strategy with different k-factors
    std::vector<double> kFactors = {0.3, 0.4, 0.5};
    
    // Create backtest engine
    BacktestEngine backtester;
    
    // Set the symbol and timeframe
    std::string symbol = "BTCUSDT";
    std::string timeframe = "1d"; // Daily
    
    // Fetch some historical data
    std::cout << "Fetching historical data for " << symbol << "..." << std::endl;
    
    std::vector<OHLCV> historicalData = exchange->fetchHistoricalData(
        symbol, timeframe, "", "");
    
    if (historicalData.empty()) {
        std::cerr << "Failed to fetch historical data" << std::endl;
        return 1;
    }
    
    std::cout << "Fetched " << historicalData.size() << " data points" << std::endl;
    
    // Run backtest for each k-factor
    for (double k : kFactors) {
        std::cout << "\nRunning backtest with k-factor = " << k << std::endl;
        
        // Configure strategy
        std::map<std::string, double> params;
        params["kFactor"] = k;
        strategy->initialize(params);
        
        // Connect strategy with exchange
        strategy->setExchange(exchange);
        
        // Run backtest
        BacktestResult result = backtester.runBacktest(strategy, historicalData, 10000.0);
        
        // Generate report
        std::string report = backtester.generateReport(result);
        std::cout << report << std::endl;
    }
    
    std::cout << "Backtesting completed!" << std::endl;
    
    return 0;
}