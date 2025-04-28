
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
#include "../include/okx_exchange.h"

void testOKXExchange() {
    std::cout << "\n=== Testing OKX Exchange ===\n" << std::endl;
    
    // Create OKX exchange instance
    auto okx = std::make_shared<OKXExchange>();
    
    // Initialize with empty API credentials (for public API endpoints)
    if (!okx->initialize("", "")) {
        std::cerr << "Failed to initialize OKX exchange" << std::endl;
        return;
    }
    
    // Test different symbol formats for OKX
    std::vector<std::string> testSymbols = {
        "BTC-USDT",    // Preferred OKX format
        "ETH-USDT",    // Another common pair
        "SOL-USDT"     // Another popular pair
    };
    
    for (const auto& symbol : testSymbols) {
        std::cout << "\nTesting symbol: " << symbol << std::endl;
        
        // Test fetching the current price
        double price = okx->getCurrentPrice(symbol);
        std::cout << "Current price of " << symbol << ": $" << price << std::endl;
        
        if (price > 0) {
            // Test fetching historical data
            std::cout << "Fetching historical data for " << symbol << "..." << std::endl;
            
            std::vector<OHLCV> historicalData = okx->fetchHistoricalData(
                symbol, "1d", "", "");
            
            std::cout << "Fetched " << historicalData.size() << " data points" << std::endl;
            
            // Print the first few data points
            int count = 0;
            for (const auto& candle : historicalData) {
                time_t timestamp = candle.timestamp;
                std::tm* tm = std::localtime(&timestamp);
                char date[11];
                std::strftime(date, sizeof(date), "%Y-%m-%d", tm);
                
                std::cout << date << ": Open=" << candle.open << ", High=" << candle.high
                          << ", Low=" << candle.low << ", Close=" << candle.close 
                          << ", Volume=" << candle.volume << std::endl;
                
                if (++count >= 5) break; // Print only first 5 candles
            }
        } else {
            std::cout << "Could not get price for " << symbol << ", skipping historical data" << std::endl;
        }
    }
}

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
        // Test OKX exchange
    testOKXExchange();    
    std::cout << "Backtesting completed!" << std::endl;
    
    return 0;
}