#ifndef VOLATILITY_BREAKOUT_H
#define VOLATILITY_BREAKOUT_H
#include "strategy.h" 

class VolatilityBreakout: public Strategy{
public:
    VolatilityBreakout(); 
    ~VolatilityBreakout() override = default;
    bool initialize(const map<string, double>& parameters) override;
    vector<Signal> processData(const vector<OHLCV>& data) override;
    string getName() const override {return "Larry William's volatility breakout"; }
private:
    double kFactor = 0.4;
    bool isLongSignal(const OHLCV& current, const OHLCV& previous) const;
    bool isShortSignal(const OHLCV& current, const OHLCV& previous) const;
};


#endif