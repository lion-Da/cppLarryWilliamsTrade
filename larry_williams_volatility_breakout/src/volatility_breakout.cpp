#include "../include/volatility_breakout.h"
#include <iostream> 

VolatilityBreakout::VolatilityBreakout() {
    parameters["kFactor"] = 0.4; 
    parameters["riskPerTrade"] = 0.02;
}

bool VolatilityBreakout::initialize(const map<string, double>& parameters)
{
    kFactor = parameters.at("kFactor");
    if (kFactor <= 0 || kFactor > 1.0)
    {
        cerr << "Invalid kFactor value: " << kFactor << ". Must be between 0 and 1." << endl; 
        return false;
    }
    return true;
}

vector<Signal> VolatilityBreakout::processData(const vector<OHLCV>& data)
{
    vector<Signal> signals; 
    if (data.size() < 2)
    {
        return signals;
    }
    for(size_t i = 1; i < data.size(); ++i)
    {
        const OHLCV& previous = data[i - 1];
        const OHLCV& current  = data[i]; 

        Signal signal; 
        signal.symbol = "";
        signal.timestamp = current.timestamp;
        if(isLongSignal(current, previous))
        {
            signal.side = OrderSide::BUY; 
            signal.suggestedPrice = current.close;
            signal.reason = "Volatility Breakout Long: Price above Open + " + to_string(kFactor) + " * Range";
            signals.push_back(signal);
        }
        else if(isShortSignal(current, previous))
        {
            signal.side = OrderSide::SELL;
            signal.suggestedPrice = current.close; 
            signal.reason = "Volatility Breakout Short: Price below Open - " + to_string(kFactor) + " * Range";
            signals.push_back(signal);
        }
    }
    return signals;
}

bool VolatilityBreakout::isLongSignal(const OHLCV& current, const OHLCV& previous) const
{
    double range = previous.calculateRange();
    double breakoutLevel = current.open + kFactor * range;
    return current.close > breakoutLevel; 

}

bool VolatilityBreakout::isShortSignal(const OHLCV& current, const OHLCV& previous) const
{
    double range = previous.calculateRange();
    double breakdownLevel = current.open - kFactor * range;
    return current.close < breakdownLevel; 
}