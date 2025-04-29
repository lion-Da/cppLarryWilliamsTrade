
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
#include "websocket_client.h"
#include "okx_exchange.h"
// Add these WebSocket testing functions
void testBinanceWebSocket() {
    std::cout << "\n=== Testing Binance WebSocket ===\n" << std::endl;
    
    // Create Binance exchange
    auto binance = std::make_shared<BinanceExchange>();
    
    // Initialize exchange
    if (!binance->initialize("", "")) {
        std::cerr << "Failed to initialize Binance exchange" << std::endl;
        return;
    }
    
    // Set up price update callback
    binance->setRealTimePriceCallback([](const std::string& symbol, double price) {
        std::cout << "Binance real-time price for " << symbol << ": $" << price << std::endl;
    });
    
    // Connect to WebSocket for Bitcoin price updates
    std::string symbol = "btcusdt";
    std::string channel = "ticker";
    
    std::cout << "Connecting to Binance WebSocket for " << symbol << " updates..." << std::endl;
    
    if (!binance->connectWebSocket(symbol, channel)) {
        std::cerr << "Failed to connect to Binance WebSocket" << std::endl;
        return;
    }
    
    std::cout << "Binance WebSocket connected. Press Enter to stop..." << std::endl;
    std::cin.get();
    
    // Disconnect WebSocket
    binance->disconnectWebSocket();
}

void testOKXWebSocket() {
    std::cout << "\n=== Testing OKX WebSocket ===\n" << std::endl;
    
    // Create OKX exchange
    auto okx = std::make_shared<OKXExchange>();
    
    // Initialize exchange
    if (!okx->initialize("", "")) {
        std::cerr << "Failed to initialize OKX exchange" << std::endl;
        return;
    }
    
    // Set up price update callback
    okx->setRealTimePriceCallback([](const std::string& symbol, double price) {
        std::cout << "OKX real-time price for " << symbol << ": $" << price << std::endl;
    });
    
    // Connect to WebSocket for Bitcoin price updates
    std::string symbol = "BTC-USDT";
    std::string channel = "tickers";
    
    std::cout << "Connecting to OKX WebSocket for " << symbol << " updates..." << std::endl;
    
    if (!okx->connectWebSocket(symbol, channel)) {
        std::cerr << "Failed to connect to OKX WebSocket" << std::endl;
        return;
    }
    
    std::cout << "OKX WebSocket connected. Press Enter to stop..." << std::endl;
    std::cin.get();
    
    // Disconnect WebSocket
    okx->disconnectWebSocket();
}

void testBothWebSockets() {
    std::cout << "\n=== Testing Both Exchanges WebSockets ===\n" << std::endl;
    
    // Create exchanges
    auto binance = std::make_shared<BinanceExchange>();
    auto okx = std::make_shared<OKXExchange>();
    
    // Initialize exchanges
    if (!binance->initialize("", "")) {
        std::cerr << "Failed to initialize Binance exchange" << std::endl;
        return;
    }
    
    if (!okx->initialize("", "")) {
        std::cerr << "Failed to initialize OKX exchange" << std::endl;
        return;
    }
    
    // Set up price update callbacks
    binance->setRealTimePriceCallback([](const std::string& symbol, double price) {
        std::cout << "Binance " << symbol << ": $" << price << std::endl;
    });
    
    okx->setRealTimePriceCallback([](const std::string& symbol, double price) {
        std::cout << "OKX " << symbol << ": $" << price << std::endl;
    });
    
    // Connect to WebSockets
    if (!binance->connectWebSocket("btcusdt", "ticker")) {
        std::cerr << "Failed to connect to Binance WebSocket" << std::endl;
    }
    
    if (!okx->connectWebSocket("BTC-USDT", "tickers")) {
        std::cerr << "Failed to connect to OKX WebSocket" << std::endl;
    }
    
    std::cout << "Both WebSockets connected. Press Enter to stop..." << std::endl;
    std::cin.get();
    
    // Disconnect WebSockets
    binance->disconnectWebSocket();
    okx->disconnectWebSocket();
}


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

    std::cout << "\nSelect WebSocket test:" << std::endl;
    std::cout << "1. Test Binance WebSocket" << std::endl;
    std::cout << "2. Test OKX WebSocket" << std::endl;
    std::cout << "3. Test Both Exchanges WebSockets" << std::endl;
    std::cout << "4. Skip WebSocket tests" << std::endl;
    
    int choice;
    std::cout << "Enter your choice (1-4): ";
    std::cin >> choice;
    std::cin.ignore(); // Clear the newline character
    
    switch (choice) {
        case 1:
            testBinanceWebSocket();
            break;
        case 2:
            testOKXWebSocket();
            break;
        case 3:
            testBothWebSockets();
            break;
        default:
            std::cout << "Skipping WebSocket tests." << std::endl;
            break;
    }
    
    std::cout << "\nAll tests completed!" << std::endl;
    
    return 0;
}