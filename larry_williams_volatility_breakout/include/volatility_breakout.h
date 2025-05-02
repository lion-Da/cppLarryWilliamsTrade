#ifndef VOLATILITY_BREAKOUT_H
#define VOLATILITY_BREAKOUT_H
#include "strategy.h"
#include <map>
#include <ctime>

class VolatilityBreakout: public Strategy {
public:
    VolatilityBreakout(); 
    ~VolatilityBreakout() override = default;
    
    bool initialize(const std::map<std::string, double>& parameters) override;
    std::vector<Signal> processData(const std::vector<OHLCV>& data) override;
    std::string getName() const override {return "Larry Williams Volatility Breakout"; }
    
private:
    // Strategy parameters
    double breakoutFactor;  // Larry Williams default is 0.25
    double profitFactor;    // Profit target as multiple of breakout range
    double stopLossFactor;  // Stop loss as multiple of breakout range
    int requiredBars;       // Minimum number of bars needed for calculation
    int excludeFirstNBars;  // Number of bars at market open to exclude
    bool useATR;            // Use ATR instead of high-low range
    int atrPeriod;          // Period for ATR calculation if used
    
    // Time-based exit settings (24-hour format)
    int exitHour;
    int exitMinute;
    
    // Struct to track trading day info
    struct TradingDay {
        std::time_t date;
        double open;
        double upperBound;
        double lowerBound;
        double rangeSize;
        bool hasLongSignal;
        bool hasShortSignal;
    };
    
    // Struct to track active trades
    struct ActiveTrade {
        std::string symbol;
        double entryPrice;
        std::time_t entryTime;
        OrderSide direction;
        double profitTarget;
        double stopLoss;
        double quantity;
    };
    
    // Helper functions
    bool isNewDay(const OHLCV& current, const OHLCV& previous) const;
    std::time_t getStartOfDay(std::time_t timestamp) const;
    std::string formatTimestamp(std::time_t timestamp) const;
    bool isLongSignal(double price, double upperBound) const;
    bool isShortSignal(double price, double lowerBound) const;
    bool isPastExitTime(const OHLCV& currentBar) const;
    double calculatePositionSize(double accountBalance, double riskPercent, 
                                double entryPrice, double stopLossPrice) const;
    double calculateATR(const std::vector<OHLCV>& data, int period) const;
    
    // Price action filters
    bool isStrongTrend(const std::vector<OHLCV>& data, size_t index, int lookback) const;
    bool isRangeExpansion(const std::vector<OHLCV>& data, size_t index) const;
    
    // Time filters
    bool isValidTradingTime(const OHLCV& bar) const;
    
    // State tracking
    std::map<std::time_t, TradingDay> tradingDays;
    std::map<std::string, ActiveTrade> activeTrades;
    
    // Internal tracking variables
    struct BarStats {
        double high;
        double low;
        std::time_t timestamp;
    };
    BarStats previousDayHigh;
    BarStats previousDayLow;
    std::map<std::string, double> upperBreakoutLevels;
    std::map<std::string, double> lowerBreakoutLevels;
    std::map<std::string, double> profitTargets;
    std::map<std::string, double> stopLosses;
    std::map<std::string, std::time_t> positionEntryTimes;
};

#endif // VOLATILITY_BREAKOUT_H