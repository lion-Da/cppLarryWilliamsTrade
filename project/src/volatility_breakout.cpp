#include "volatility_breakout.h"
#include <iostream>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

VolatilityBreakout::VolatilityBreakout() {
    // Initialize with default parameters
    breakoutFactor = 0.25;  // Larry Williams' default value
    profitFactor = 2.0;     // Take profit at 2x the breakout range
    stopLossFactor = 1.0;   // Stop loss at 1x the breakout range
    requiredBars = 2;       // Minimum number of bars needed
    excludeFirstNBars = 1;  // Skip first bar of the day (usually erratic)
    useATR = false;         // Default to simple high-low range
    atrPeriod = 14;         // Standard ATR period
    exitHour = 21;          // Exit time (21:59)
    exitMinute = 59;
    
    // Initialize tracking data
    previousDayHigh.high = -1;
    previousDayHigh.timestamp = 0;
    previousDayLow.low = 999999999;
    previousDayLow.timestamp = 0;
    
    // Store in parameters map for initialization
    parameters["breakoutFactor"] = breakoutFactor;
    parameters["profitFactor"] = profitFactor;
    parameters["stopLossFactor"] = stopLossFactor;
    parameters["requiredBars"] = static_cast<double>(requiredBars);
    parameters["excludeFirstNBars"] = static_cast<double>(excludeFirstNBars);
    parameters["useATR"] = useATR ? 1.0 : 0.0;
    parameters["atrPeriod"] = static_cast<double>(atrPeriod);
    parameters["exitHour"] = static_cast<double>(exitHour);
    parameters["exitMinute"] = static_cast<double>(exitMinute);
}

bool VolatilityBreakout::initialize(const std::map<std::string, double>& params) {
    // Update parameters from the provided map
    for (const auto& param : params) {
        parameters[param.first] = param.second;
    }
    
    // Extract parameters with validation and fallback to defaults
    if (parameters.count("breakoutFactor") > 0) breakoutFactor = parameters["breakoutFactor"];
    if (parameters.count("profitFactor") > 0) profitFactor = parameters["profitFactor"];
    if (parameters.count("stopLossFactor") > 0) stopLossFactor = parameters["stopLossFactor"];
    if (parameters.count("requiredBars") > 0) requiredBars = static_cast<int>(parameters["requiredBars"]);
    if (parameters.count("excludeFirstNBars") > 0) excludeFirstNBars = static_cast<int>(parameters["excludeFirstNBars"]);
    if (parameters.count("useATR") > 0) useATR = (parameters["useATR"] > 0.5);
    if (parameters.count("atrPeriod") > 0) atrPeriod = static_cast<int>(parameters["atrPeriod"]);
    if (parameters.count("exitHour") > 0) exitHour = static_cast<int>(parameters["exitHour"]);
    if (parameters.count("exitMinute") > 0) exitMinute = static_cast<int>(parameters["exitMinute"]);
    
    std::cout << "Initialized Larry Williams Volatility Breakout strategy with:" << std::endl;
    std::cout << "- Breakout Factor: " << breakoutFactor << std::endl;
    std::cout << "- Profit Factor: " << profitFactor << std::endl;
    std::cout << "- Stop Loss Factor: " << stopLossFactor << std::endl;
    std::cout << "- Using ATR: " << (useATR ? "Yes" : "No") << std::endl;
    if (useATR) std::cout << "- ATR Period: " << atrPeriod << std::endl;
    std::cout << "- Exit Time: " << exitHour << ":" << exitMinute << std::endl;
    
    return true;
}

std::vector<Signal> VolatilityBreakout::processData(const std::vector<OHLCV>& data) {
    std::vector<Signal> signals;
    
    // Need at least requiredBars bars to calculate
    if (data.size() < requiredBars) {
        return signals;
    }
    
    // Get the current symbol from the data
    std::string symbol = "UNKNOWN";
    if (!data.empty() && !data[0].symbol.empty()) {
        symbol = data[0].symbol;
    } else if (exchange) {
        // In real implementation, extract symbol from data or context
        symbol = "BTC-USDT"; // Default for testing
    }
    
    // Calculate ATR if needed
    double atr = 0.0;
    if (useATR && data.size() >= atrPeriod) {
        atr = calculateATR(data, atrPeriod);
    }
    
    // Find day boundaries in the data
    std::vector<size_t> dayBoundaries;
    for (size_t i = 1; i < data.size(); i++) {
        if (isNewDay(data[i], data[i-1])) {
            dayBoundaries.push_back(i);
        }
    }
    
    // If we don't have day boundaries yet, initialize from scratch
    if (dayBoundaries.empty() && previousDayHigh.timestamp == 0) {
        // Find the highest high and lowest low from the previous day
        std::time_t currentDay = getStartOfDay(data.back().timestamp);
        std::time_t previousDay = currentDay - 86400; // Previous day
        
        double prevDayHigh = -1;
        double prevDayLow = 999999999;
        
        for (const auto& bar : data) {
            std::time_t barDay = getStartOfDay(bar.timestamp);
            if (barDay == previousDay) {
                prevDayHigh = std::max(prevDayHigh, bar.high);
                prevDayLow = std::min(prevDayLow, bar.low);
            }
        }
        
        // Initialize with previous day data
        previousDayHigh.high = prevDayHigh;
        previousDayHigh.timestamp = previousDay;
        previousDayLow.low = prevDayLow;
        previousDayLow.timestamp = previousDay;
    }
    
    // Process each day boundary
    for (size_t i = 0; i < dayBoundaries.size(); i++) {
        size_t dayStart = dayBoundaries[i];
        
        // Find previous day's data
        double prevDayHigh = -1;
        double prevDayLow = 999999999;
        std::time_t currentDay = getStartOfDay(data[dayStart].timestamp);
        std::time_t previousDay = currentDay - 86400;
        
        // Determine previous day's high and low
        for (const auto& bar : data) {
            std::time_t barDay = getStartOfDay(bar.timestamp);
            if (barDay == previousDay) {
                prevDayHigh = std::max(prevDayHigh, bar.high);
                prevDayLow = std::min(prevDayLow, bar.low);
            }
        }
        
        // Update tracking
        previousDayHigh.high = prevDayHigh;
        previousDayHigh.timestamp = previousDay;
        previousDayLow.low = prevDayLow;
        previousDayLow.timestamp = previousDay;
        
        // Calculate range (either simple range or ATR)
        double rangeSize;
        if (useATR && atr > 0) {
            rangeSize = atr * breakoutFactor;
        } else {
            rangeSize = (prevDayHigh - prevDayLow) * breakoutFactor;
        }
        
        // Calculate breakout levels for today
        double upperBound = data[dayStart].open + rangeSize;
        double lowerBound = data[dayStart].open - rangeSize;
        
        // Store in maps
        upperBreakoutLevels[symbol] = upperBound;
        lowerBreakoutLevels[symbol] = lowerBound;
        
        // Store in trading day map
        std::time_t dayTimestamp = getStartOfDay(data[dayStart].timestamp);
        tradingDays[dayTimestamp] = TradingDay{
            .date = dayTimestamp,
            .open = data[dayStart].open,
            .upperBound = upperBound,
            .lowerBound = lowerBound,
            .rangeSize = rangeSize,
            .hasLongSignal = false,
            .hasShortSignal = false
        };
        
        std::cout << "New day detected " << formatTimestamp(dayTimestamp) << ". Breakout levels for " << symbol 
                  << ": Upper=" << upperBound << ", Lower=" << lowerBound << std::endl;
    }
    
    // Process each bar for entry and exit signals
    for (size_t i = excludeFirstNBars; i < data.size(); i++) {
        const OHLCV& currentBar = data[i];
        
        // Skip if outside valid trading hours
        if (!isValidTradingTime(currentBar)) {
            continue;
        }
        
        // Find which trading day this bar belongs to
        std::time_t currentDay = getStartOfDay(currentBar.timestamp);
        
        // Check for breakout entry signals
        if (tradingDays.count(currentDay) > 0) {
            TradingDay& day = tradingDays[currentDay];
            
            // Only generate signals if we haven't already for this day
            // Long entry
            if (!day.hasLongSignal && isLongSignal(currentBar.high, day.upperBound)) {
                // Check additional filters
                bool takeTrade = true;
                
                // Optional: Add trend filter
                if (parameters.count("trendFilter") > 0 && parameters["trendFilter"] > 0.5) {
                    takeTrade = isStrongTrend(data, i, 5);
                }
                
                // Optional: Add range expansion filter
                if (takeTrade && parameters.count("rangeFilter") > 0 && parameters["rangeFilter"] > 0.5) {
                    takeTrade = isRangeExpansion(data, i);
                }
                
                if (takeTrade) {
                    day.hasLongSignal = true;
                    
                    // Calculate entry price (at the breakout level)
                    double entryPrice = day.upperBound;
                    double rangeSize = day.rangeSize;
                    
                    // Calculate stop loss
                    double stopLoss = entryPrice - (rangeSize * stopLossFactor);
                    
                    // Calculate profit target
                    double profitTarget = entryPrice + (rangeSize * profitFactor);
                    
                    // Create position size - risk 1% of account per trade (10000 HKD starter)
                    double accountSize = 10000.0; // Your initial capital
                    double riskPercent = 0.01;    // 1% risk
                    double positionSize = calculatePositionSize(accountSize, riskPercent, entryPrice, stopLoss);
                    
                    // Create signal
                    Signal signal;
                    signal.symbol = symbol;
                    signal.side = OrderSide::BUY;
                    signal.suggestedPrice = entryPrice;
                    signal.suggestedQuantity = positionSize;
                    signal.timestamp = currentBar.timestamp;
                    signal.reason = "Volatility Breakout Long";
                    
                    // Store trade data
                    ActiveTrade trade;
                    trade.symbol = symbol;
                    trade.entryPrice = entryPrice;
                    trade.entryTime = currentBar.timestamp;
                    trade.direction = OrderSide::BUY;
                    trade.profitTarget = profitTarget;
                    trade.stopLoss = stopLoss;
                    trade.quantity = positionSize;
                    activeTrades[symbol] = trade;
                    
                    // Store for legacy code compatibility
                    profitTargets[symbol] = profitTarget;
                    stopLosses[symbol] = stopLoss;
                    positionEntryTimes[symbol] = currentBar.timestamp;
                    
                    signals.push_back(signal);
                    
                    std::cout << "LONG signal for " << symbol << " at " << entryPrice 
                              << " (Target: " << profitTarget << ", Stop: " << stopLoss 
                              << ", Size: " << positionSize << ")" << std::endl;
                }
            }
            
            // Short entry
            if (!day.hasShortSignal && isShortSignal(currentBar.low, day.lowerBound)) {
                // Check additional filters
                bool takeTrade = true;
                
                // Optional: Add trend filter
                if (parameters.count("trendFilter") > 0 && parameters["trendFilter"] > 0.5) {
                    takeTrade = isStrongTrend(data, i, 5);
                }
                
                // Optional: Add range expansion filter
                if (takeTrade && parameters.count("rangeFilter") > 0 && parameters["rangeFilter"] > 0.5) {
                    takeTrade = isRangeExpansion(data, i);
                }
                
                if (takeTrade) {
                    day.hasShortSignal = true;
                    
                    // Calculate entry price (at the breakout level)
                    double entryPrice = day.lowerBound;
                    double rangeSize = day.rangeSize;
                    
                    // Calculate stop loss
                    double stopLoss = entryPrice + (rangeSize * stopLossFactor);
                    
                    // Calculate profit target
                    double profitTarget = entryPrice - (rangeSize * profitFactor);
                    
                    // Create position size - risk 1% of account per trade
                    double accountSize = 10000.0; // Your initial capital
                    double riskPercent = 0.01;    // 1% risk
                    double positionSize = calculatePositionSize(accountSize, riskPercent, entryPrice, stopLoss);
                    
                    // Create signal
                    Signal signal;
                    signal.symbol = symbol;
                    signal.side = OrderSide::SELL;
                    signal.suggestedPrice = entryPrice;
                    signal.suggestedQuantity = positionSize;
                    signal.timestamp = currentBar.timestamp;
                    signal.reason = "Volatility Breakout Short";
                    
                    // Store trade data
                    ActiveTrade trade;
                    trade.symbol = symbol;
                    trade.entryPrice = entryPrice;
                    trade.entryTime = currentBar.timestamp;
                    trade.direction = OrderSide::SELL;
                    trade.profitTarget = profitTarget;
                    trade.stopLoss = stopLoss;
                    trade.quantity = positionSize;
                    activeTrades[symbol] = trade;
                    
                    // Store for legacy code compatibility
                    profitTargets[symbol] = profitTarget;
                    stopLosses[symbol] = stopLoss;
                    positionEntryTimes[symbol] = currentBar.timestamp;
                    
                    signals.push_back(signal);
                    
                    std::cout << "SHORT signal for " << symbol << " at " << entryPrice 
                              << " (Target: " << profitTarget << ", Stop: " << stopLoss 
                              << ", Size: " << positionSize << ")" << std::endl;
                }
            }
        }
        
        // Check for exit signals if we have an active trade
        if (activeTrades.count(symbol) > 0) {
            const ActiveTrade& trade = activeTrades[symbol];
            
            // Check for profit target hit
            if ((trade.direction == OrderSide::BUY && currentBar.high >= trade.profitTarget) ||
                (trade.direction == OrderSide::SELL && currentBar.low <= trade.profitTarget)) {
                
                Signal exitSignal;
                exitSignal.symbol = symbol;
                exitSignal.side = (trade.direction == OrderSide::BUY) ? OrderSide::SELL : OrderSide::BUY;
                exitSignal.suggestedPrice = trade.profitTarget;
                exitSignal.suggestedQuantity = trade.quantity;
                exitSignal.timestamp = currentBar.timestamp;
                exitSignal.reason = "Take Profit";
                
                signals.push_back(exitSignal);
                
                std::cout << "PROFIT TARGET HIT for " << symbol << " at " << trade.profitTarget << std::endl;
                
                // Remove from active trades
                activeTrades.erase(symbol);
            }
            
            // Check for stop loss hit
            else if ((trade.direction == OrderSide::BUY && currentBar.low <= trade.stopLoss) ||
                     (trade.direction == OrderSide::SELL && currentBar.high >= trade.stopLoss)) {
                
                Signal exitSignal;
                exitSignal.symbol = symbol;
                exitSignal.side = (trade.direction == OrderSide::BUY) ? OrderSide::SELL : OrderSide::BUY;
                exitSignal.suggestedPrice = trade.stopLoss;
                exitSignal.suggestedQuantity = trade.quantity;
                exitSignal.timestamp = currentBar.timestamp;
                exitSignal.reason = "Stop Loss";
                
                signals.push_back(exitSignal);
                
                std::cout << "STOP LOSS HIT for " << symbol << " at " << trade.stopLoss << std::endl;
                
                // Remove from active trades
                activeTrades.erase(symbol);
            }
            
            // Check for time-based exit
            else if (isPastExitTime(currentBar)) {
                Signal exitSignal;
                exitSignal.symbol = symbol;
                exitSignal.side = (trade.direction == OrderSide::BUY) ? OrderSide::SELL : OrderSide::BUY;
                exitSignal.suggestedPrice = currentBar.close;
                exitSignal.suggestedQuantity = trade.quantity;
                exitSignal.timestamp = currentBar.timestamp;
                exitSignal.reason = "Time Exit";
                
                signals.push_back(exitSignal);
                
                std::cout << "TIME-BASED EXIT for " << symbol << " at " << currentBar.close << std::endl;
                
                // Remove from active trades
                activeTrades.erase(symbol);
            }
        }
    }
    
    return signals;
}

bool VolatilityBreakout::isNewDay(const OHLCV& current, const OHLCV& previous) const {
    // Get day component of timestamps
    std::time_t currentDayStart = getStartOfDay(current.timestamp);
    std::time_t previousDayStart = getStartOfDay(previous.timestamp);
    
    // Compare if they're different days
    return currentDayStart != previousDayStart;
}

std::time_t VolatilityBreakout::getStartOfDay(std::time_t timestamp) const {
    // Convert timestamp to tm struct
    std::tm* tmTime = std::localtime(&timestamp);
    
    // Set time components to midnight (00:00:00)
    tmTime->tm_hour = 0;
    tmTime->tm_min = 0;
    tmTime->tm_sec = 0;
    
    // Convert back to time_t
    return std::mktime(tmTime);
}

std::string VolatilityBreakout::formatTimestamp(std::time_t timestamp) const {
    char buffer[80];
    std::tm* timeinfo = std::localtime(&timestamp);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return std::string(buffer);
}

bool VolatilityBreakout::isLongSignal(double price, double upperBound) const {
    return price > upperBound;
}

bool VolatilityBreakout::isShortSignal(double price, double lowerBound) const {
    return price < lowerBound;
}

bool VolatilityBreakout::isPastExitTime(const OHLCV& currentBar) const {
    std::time_t barTime = currentBar.timestamp;
    std::tm* barTm = std::localtime(&barTime);
    
    // Check if we're past the exit time
    return (barTm->tm_hour > exitHour || 
            (barTm->tm_hour == exitHour && barTm->tm_min >= exitMinute));
}

double VolatilityBreakout::calculatePositionSize(double accountBalance, double riskPercent, 
                                                double entryPrice, double stopLossPrice) const {
    double riskAmount = accountBalance * riskPercent;
    double priceDifference = std::abs(entryPrice - stopLossPrice);
    
    // Avoid division by zero
    if (priceDifference < 0.00001) {
        priceDifference = 0.00001;
    }
    
    // Calculate position size
    double positionSize = riskAmount / priceDifference;
    
    // For higher-priced assets, we might need to round to fewer decimal places
    if (entryPrice > 1000) {
        positionSize = std::floor(positionSize * 1000) / 1000; // Round to 3 decimal places
    } else if (entryPrice > 100) {
        positionSize = std::floor(positionSize * 10000) / 10000; // Round to 4 decimal places
    } else {
        positionSize = std::floor(positionSize * 100000) / 100000; // Round to 5 decimal places
    }
    
    return positionSize;
}

double VolatilityBreakout::calculateATR(const std::vector<OHLCV>& data, int period) const {
    if (data.size() < period + 1) {
        return 0.0;
    }
    
    // Calculate initial TR values
    std::vector<double> trValues;
    for (size_t i = 1; i < data.size(); i++) {
        double trueHigh = std::max(data[i].high, data[i-1].close);
        double trueLow = std::min(data[i].low, data[i-1].close);
        double tr = trueHigh - trueLow;
        trValues.push_back(tr);
    }
    
    // Calculate ATR using simple moving average
    double sum = 0.0;
    for (int i = 0; i < period && i < trValues.size(); i++) {
        sum += trValues[trValues.size() - 1 - i];
    }
    
    return sum / std::min(period, static_cast<int>(trValues.size()));
}

bool VolatilityBreakout::isStrongTrend(const std::vector<OHLCV>& data, size_t index, int lookback) const {
    if (index < lookback) {
        return false;
    }
    
    // For long signals: check if price is in an uptrend (higher highs, higher lows)
    // For short signals: check if price is in a downtrend (lower highs, lower lows)
    
    bool uptrend = true;
    bool downtrend = true;
    
    for (int i = 1; i <= lookback; i++) {
        if (data[index - i].high <= data[index - i - 1].high) {
            uptrend = false;
        }
        if (data[index - i].low <= data[index - i - 1].low) {
            uptrend = false;
        }
        if (data[index - i].high >= data[index - i - 1].high) {
            downtrend = false;
        }
        if (data[index - i].low >= data[index - i - 1].low) {
            downtrend = false;
        }
    }
    
    return uptrend || downtrend;
}

bool VolatilityBreakout::isRangeExpansion(const std::vector<OHLCV>& data, size_t index) const {
    if (index < 5) {
        return false;
    }
    
    // Calculate the average range of the last 5 bars
    double totalRange = 0.0;
    for (int i = 1; i <= 5; i++) {
        totalRange += (data[index - i].high - data[index - i].low);
    }
    double avgRange = totalRange / 5.0;
    
    // Check if current bar's range is larger than average
    double currentRange = data[index].high - data[index].low;
    
    return currentRange > avgRange * 1.2; // 20% higher than average
}

bool VolatilityBreakout::isValidTradingTime(const OHLCV& bar) const {
    std::time_t barTime = bar.timestamp;
    std::tm* barTm = std::localtime(&barTime);
    
    // Skip bars in the first hour of trading (often erratic)
    // This will depend on the market opening time
    // For example, if market opens at 9:30, skip until 10:30
    if (parameters.count("skipFirstHour") > 0 && parameters.at("skipFirstHour")) {
        int marketOpenHour = 9; // Adjust based on your market
        if (barTm->tm_hour == marketOpenHour) {
            return false;
        }
    }
    
    // Skip bars near market close
    // For example, if market closes at 16:00, skip after 15:30
    if (parameters.count("avoidLastHalfHour") > 0 && parameters.at("avoidLastHalfHour") > 0.5) {
        int marketCloseHour = 16; // Adjust based on your market
        if (barTm->tm_hour == marketCloseHour - 1 && barTm->tm_min >= 30) {
            return false;
        }
    }
    
    return true;
}