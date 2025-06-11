#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>
#include "data_types.h"
#include "exchange.h"
#include "binance_exchange.h"
#include "volatility_breakout.h"
#include "backtest_engine.h"
#include "websocket_client.h"
#include "okx_exchange.h"
#include "bybit_exchange.h"
#include "env_loader.h"
#include <unordered_map>
// Global flag for termination
volatile sig_atomic_t g_running = 1;

// Signal handler for clean shutdown
void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    g_running = 0;
}

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"
void testGetOkBbBtc()
{
    std::string okx_symbol = "BTC-USDT";
    std::string bybit_symbol = "BTCUSDT";
    std::string okx_channel = "tickers";
    std::string bybit_channel = "tickers";

    // Create exchanges
    auto okx = std::make_shared<OKXExchange>();
    auto bybit = std::make_shared<BybitExchange>();
    // Initialize exchanges
    if (!okx->initialize("", "")) {
        std::cerr << "Failed to initialize OKX exchange" << std::endl;
        return;
    }

    if (!bybit->initialize("", "")) {
        std::cerr << "Failed to initialize Bybit exchange" << std::endl;
        return;
    }
    std::pair<std::string, double> okx_prices;
    std::pair<std::string, double> bybit_prices;

    auto price_strategy = [&]()
    {
        std::cout << std::fixed << std::setprecision(8) 
            << "BTCUSDT OKX(" 
            << YELLOW << okx_prices.first << "," << okx_prices.second  << RESET
            << ")  Bybit(" 
            << YELLOW << bybit_prices.first << "," << bybit_prices.second << RESET
            << ")" << std::endl;
    };
    // Set up price update callbacks
    okx->setRealTimePriceCallback([&](const std::string& symbol, double price, const std::string& ts) {
        okx_prices.first = ts;
        okx_prices.second = price;
        price_strategy();
        
    });

    bybit->setRealTimePriceCallback([&](const std::string& symbol, double price, const std::string& ts) {
        bybit_prices.first = ts;
        bybit_prices.second = price;
        price_strategy();
    });
    
    // Connect to WebSockets
    if (!okx->connectWebSocket(okx_symbol, okx_channel)) {
        std::cerr << "Failed to connect to OKX WebSocket" << std::endl;
    }

    if (!bybit->connectWebSocket(bybit_symbol, bybit_channel)) {
        std::cerr << "Failed to connect to Bybit WebSocket" << std::endl;
    }
    
    std::cout << "Both WebSockets connected. Press Enter to stop..." << std::endl;
    std::cin.get();
    
    // Disconnect WebSockets
    okx->disconnectWebSocket();
    bybit->disconnectWebSocket();
}


// New function to run the actual trading bot with your improved strategy
void runOKXTradingBot() {
    // Register signal handler for clean shutdown
    signal(SIGINT, signal_handler);

    std::cout << "\n=== Starting Larry Williams Volatility Breakout Trading Bot ===\n" << std::endl;
    
    // Ask for API credentials
    std::string apiKey, apiSecret, passphrase;
    
    std::cout << "Enter OKX API Key (or press Enter to use empty key for testing): ";
    std::getline(std::cin, apiKey);
    
    if (!apiKey.empty()) {
        std::cout << "Enter OKX API Secret: ";
        std::getline(std::cin, apiSecret);
        
        std::cout << "Enter OKX Passphrase: ";
        std::getline(std::cin, passphrase);
    }
    
    // Initialize exchange
    auto okx = std::make_shared<OKXExchange>();
    if (!okx->initialize(apiKey, apiSecret)) {
        std::cerr << "Failed to initialize OKX exchange" << std::endl;
        return;
    }
    
    if (!passphrase.empty()) {
        okx->setPassphrase(passphrase);
    }
    
    // Ask for trading parameters
    std::string symbolInput;
    std::cout << "Enter symbol to trade (default: BTC-USDT): ";
    std::getline(std::cin, symbolInput);
    std::string symbol = symbolInput.empty() ? "BTC-USDT" : symbolInput;
    
    std::string timeframeInput;
    std::cout << "Enter timeframe (1m, 5m, 15m, 1h, 4h, 1d - default: 1h): ";
    std::getline(std::cin, timeframeInput);
    std::string timeframe = timeframeInput.empty() ? "1h" : timeframeInput;
    
    std::string breakoutFactorInput;
    std::cout << "Enter breakout factor (0.1-1.0, default: 0.3): ";
    std::getline(std::cin, breakoutFactorInput);
    double breakoutFactor = breakoutFactorInput.empty() ? 0.3 : std::stod(breakoutFactorInput);
    
    std::string initialCapitalInput;
    std::cout << "Enter initial capital (default: 1000 HKD): ";
    std::getline(std::cin, initialCapitalInput);
    double initialCapital = initialCapitalInput.empty() ? 1000.0 : std::stod(initialCapitalInput);
    
    std::string riskPerTradeInput;
    std::cout << "Enter risk per trade (% of capital, default: 1): ";
    std::getline(std::cin, riskPerTradeInput);
    double riskPerTrade = riskPerTradeInput.empty() ? 1.0 : std::stod(riskPerTradeInput);
    
    // Initialize strategy
    auto strategy = std::make_shared<VolatilityBreakout>();
    
    // Configure strategy parameters
    std::map<std::string, double> params;
    params["breakoutFactor"] = breakoutFactor;
    params["profitFactor"] = 2.0;      // Take profit at 2x the entry range
    params["stopLossFactor"] = 1.0;    // Stop loss at 1x the entry range
    params["exitHour"] = 21;           // Exit at 21:59
    params["exitMinute"] = 59;
    params["useATR"] = 0;              // Use simple range initially
    
    // Initialize strategy
    strategy->initialize(params);
    strategy->setExchange(okx);
    
    // Create vector with the selected symbol
    std::vector<std::string> symbols = {symbol};
    
    // Map to store historical data
    std::map<std::string, std::vector<OHLCV>> symbolData;
    
    // Initialize with historical data
    std::cout << "Fetching historical data for " << symbol << "..." << std::endl;
    
    // Fetch 5 days of historical data
    std::time_t now = std::time(nullptr);
    std::time_t fiveDaysAgo = now - (5 * 24 * 60 * 60);
    
    std::vector<OHLCV> initialData = okx->fetchHistoricalData(
        symbol,
        timeframe,
        std::to_string(fiveDaysAgo * 1000),
        std::to_string(now * 1000)
    );
    
    // Add symbol to the data for clarity
    for (auto& candle : initialData) {
        candle.symbol = symbol;
    }
    
    symbolData[symbol] = initialData;
    std::cout << "Loaded " << initialData.size() << " historical bars for " << symbol << std::endl;
    
    // Connect to WebSocket for real-time updates
    std::string channel;
    if (timeframe == "1m") channel = "candle1m";
    else if (timeframe == "5m") channel = "candle5m";
    else if (timeframe == "15m") channel = "candle15m";
    else if (timeframe == "1h") channel = "candle1H";
    else if (timeframe == "4h") channel = "candle4H";
    else if (timeframe == "1d") channel = "candle1D";
    else channel = "candle1H"; // Default
    
    bool connected = okx->connectWebSocket(symbol, channel);
    std::cout << "WebSocket connection for " << symbol << ": " 
              << (connected ? "SUCCESS" : "FAILED") << std::endl;
    
    // Set up candle callback
    okx->setRealTimeCandleCallback([&](const OHLCV& candle) {
        // Add symbol to candle if not present
        OHLCV updatedCandle = candle;
        if (updatedCandle.symbol.empty()) {
            updatedCandle.symbol = symbol;
        }
        
        std::cout << "Real-time candle for " << updatedCandle.symbol << ": "
                  << "O=" << updatedCandle.open << ", H=" << updatedCandle.high 
                  << ", L=" << updatedCandle.low << ", C=" << updatedCandle.close 
                  << ", V=" << updatedCandle.volume << std::endl;
        
        // Add to our data
        bool updated = false;
        
        // Update existing candle if same timestamp
        for (auto& existingCandle : symbolData[symbol]) {
            if (existingCandle.timestamp == updatedCandle.timestamp) {
                existingCandle = updatedCandle;
                updated = true;
                break;
            }
        }
        
        // Add as new candle if not updated
        if (!updated) {
            symbolData[symbol].push_back(updatedCandle);
            
            // Sort by timestamp to ensure correct order
            std::sort(symbolData[symbol].begin(), symbolData[symbol].end(), 
                     [](const OHLCV& a, const OHLCV& b) {
                         return a.timestamp < b.timestamp;
                     });
        }
        
        // Process with strategy
        std::vector<Signal> signals = strategy->processData(symbolData[symbol]);
        
        // Execute signals (if real API key provided)
        for (const auto& signal : signals) {
            std::cout << "\n====== TRADE SIGNAL ======" << std::endl;
            std::cout << "Symbol: " << signal.symbol << std::endl;
            std::cout << "Action: " << (signal.side == OrderSide::BUY ? "BUY" : "SELL") << std::endl;
            std::cout << "Price: " << signal.suggestedPrice << std::endl;
            std::cout << "Quantity: " << signal.suggestedQuantity << std::endl;
            std::cout << "Reason: " << signal.reason << std::endl;
            std::cout << "==========================\n" << std::endl;
            
            // Execute order if we have real API credentials
            if (!apiKey.empty() && !apiSecret.empty() && !passphrase.empty()) {
                bool success = false;
                if (signal.side == OrderSide::BUY) {
                    success = okx->placeBuyOrder(
                        signal.symbol, 
                        signal.suggestedQuantity, 
                        signal.suggestedPrice
                    );
                } else {
                    success = okx->placeSellOrder(
                        signal.symbol, 
                        signal.suggestedQuantity, 
                        signal.suggestedPrice
                    );
                }
                
                std::cout << "Order execution " << (success ? "SUCCESSFUL" : "FAILED") << std::endl;
            } else {
                std::cout << "PAPER TRADING: No real order placed (API credentials not provided)" << std::endl;
            }
        }
    });
    
    std::cout << "\nTrading bot is now running. Press Ctrl+C to stop." << std::endl;
    
    // Main loop
    while (g_running) {
        // Sleep for a minute
        std::this_thread::sleep_for(std::chrono::minutes(1));
        
        // Periodically refresh historical data to ensure we don't miss anything
        if (g_running) {
            std::time_t now = std::time(nullptr);
            std::time_t oneDayAgo = now - (24 * 60 * 60);
            
            std::vector<OHLCV> refreshData = okx->fetchHistoricalData(
                symbol,
                timeframe,
                std::to_string(oneDayAgo * 1000),
                std::to_string(now * 1000)
            );
            
            // Add symbol to data
            for (auto& candle : refreshData) {
                candle.symbol = symbol;
            }
            
            // Update existing candles and add new ones
            for (const auto& newCandle : refreshData) {
                bool found = false;
                
                for (auto& existingCandle : symbolData[symbol]) {
                    if (existingCandle.timestamp == newCandle.timestamp) {
                        existingCandle = newCandle;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    symbolData[symbol].push_back(newCandle);
                }
            }
            
            // Sort by timestamp to ensure correct order
            std::sort(symbolData[symbol].begin(), symbolData[symbol].end(), 
                     [](const OHLCV& a, const OHLCV& b) {
                         return a.timestamp < b.timestamp;
                     });
        }
    }
    
    std::cout << "Shutting down trading bot..." << std::endl;
    
    // Disconnect WebSocket
    okx->disconnectWebSocket();
    
    std::cout << "Trading bot stopped." << std::endl;
}

// Backtesting with the improved strategy
void runBacktestImprovedStrategy() {
    std::cout << "\n=== Backtesting Improved Volatility Breakout Strategy ===\n" << std::endl;
    
    // Ask for exchange to use
    std::string exchangeInput;
    std::cout << "Select exchange for backtesting (1 for Binance, 2 for OKX, default: 2): ";
    std::getline(std::cin, exchangeInput);
    int exchangeChoice = exchangeInput.empty() ? 2 : std::stoi(exchangeInput);
    
    // Create exchange instance
    std::shared_ptr<Exchange> exchange;
    if (exchangeChoice == 1) {
        exchange = std::make_shared<BinanceExchange>();
    } else {
        exchange = std::make_shared<OKXExchange>();
    }
    
    // Initialize exchange (with empty API keys for backtesting)
    if (!exchange->initialize("", "")) {
        std::cerr << "Failed to initialize exchange" << std::endl;
        return;
    }
    
    // Ask for symbol to backtest
    std::string symbolInput;
    if (exchangeChoice == 1) {
        std::cout << "Enter symbol to backtest (default: BTCUSDT): ";
    } else {
        std::cout << "Enter symbol to backtest (default: BTC-USDT): ";
    }
    std::getline(std::cin, symbolInput);
    std::string symbol = symbolInput.empty() ? 
                        (exchangeChoice == 1 ? "BTCUSDT" : "BTC-USDT") : 
                        symbolInput;
    
    // Ask for timeframe
    std::string timeframeInput;
    std::cout << "Enter timeframe (1m, 5m, 15m, 1h, 4h, 1d - default: 1d): ";
    std::getline(std::cin, timeframeInput);
    std::string timeframe = timeframeInput.empty() ? "1d" : timeframeInput;
    
    // Create strategy instance
    auto strategy = std::make_shared<VolatilityBreakout>();
    
    // Ask for breakout factors to test
    std::string breakoutFactorsInput;
    std::cout << "Enter breakout factors to test, separated by commas (default: 0.3,0.4,0.5): ";
    std::getline(std::cin, breakoutFactorsInput);
    
    std::vector<double> breakoutFactors;
    if (breakoutFactorsInput.empty()) {
        breakoutFactors = {0.3, 0.4, 0.5};
    } else {
        std::stringstream ss(breakoutFactorsInput);
        std::string item;
        while (std::getline(ss, item, ',')) {
            breakoutFactors.push_back(std::stod(item));
        }
    }
    
    // Create backtest engine
    BacktestEngine backtester;
    
    // Ask for initial capital
    std::string initialCapitalInput;
    std::cout << "Enter initial capital for backtest (default: 10000): ";
    std::getline(std::cin, initialCapitalInput);
    double initialCapital = initialCapitalInput.empty() ? 10000.0 : std::stod(initialCapitalInput);
    
    // Fetch historical data
    std::cout << "Fetching historical data for " << symbol << "..." << std::endl;
    
    std::vector<OHLCV> historicalData = exchange->fetchHistoricalData(
        symbol, timeframe, "", "");
    
    if (historicalData.empty()) {
        std::cerr << "Failed to fetch historical data" << std::endl;
        return;
    }
    
    // Add symbol to data for clarity
    for (auto& candle : historicalData) {
        candle.symbol = symbol;
    }
    
    std::cout << "Fetched " << historicalData.size() << " data points" << std::endl;
    
    // Run backtest for each breakout factor
    for (double factor : breakoutFactors) {
        std::cout << "\nRunning backtest with breakoutFactor = " << factor << std::endl;
        
        // Configure strategy
        std::map<std::string, double> params;
        params["breakoutFactor"] = factor;
        params["profitFactor"] = 2.0;
        params["stopLossFactor"] = 1.0;
        strategy->initialize(params);
        
        // Connect strategy with exchange
        strategy->setExchange(exchange);
        
        // Run backtest
        BacktestResult result = backtester.runBacktest(strategy, historicalData, initialCapital);
        
        // Generate report
        std::string report = backtester.generateReport(result);
        std::cout << report << std::endl;
    }
    
    std::cout << "Backtesting completed!" << std::endl;
}

int main() {
    EnvLoader::loadEnv(); // Load environment variables if needed
    std::cout << "Larry Williams Volatility Breakout Strategy" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    std::cout << "\nSelect an option:" << std::endl;
    std::cout << "1. Test WebSocket connections" << std::endl;
    std::cout << "2. Exit" << std::endl;
    
    int choice;
    std::cout << "Enter your choice (1-2): ";
    std::cin >> choice;
    std::cin.ignore(); // Clear the newline character
    
    switch (choice) {
    
        case 1: {
            // WebSocket tests submenu
            std::cout << "\nSelect WebSocket test:" << std::endl;
            std::cout << "1. Get OKX Bybit BTC Realtime Price By Websockets" << std::endl;
            std::cout << "2. Back to main menu" << std::endl;
            
            int wsChoice;
            std::cout << "Enter your choice (1-2): ";
            std::cin >> wsChoice;
            std::cin.ignore(); // Clear the newline character
            
            switch (wsChoice) {
                case 1:
                    testGetOkBbBtc();
                    break;
                default:
                    std::cout << "Returning to main menu." << std::endl;
                    break;
            }
            break;
        }
        
            
        case 2:
            std::cout << "Exiting program." << std::endl;
            return 0;
            
        default:
            std::cout << "Invalid choice. Exiting." << std::endl;
            return 1;
    }
    
    std::cout << "\nAll operations completed!" << std::endl;
    
    return 0;
}