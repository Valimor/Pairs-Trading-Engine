#ifndef TRADING_ENGINE_HPP
#define TRADING_ENGINE_HPP

#include <quant/core/data_provider.hpp>
#include <quant/core/types.hpp>
#include <memory>

namespace qr_engine {

class TradingEngine {
public:
    // We pass a reference to the Interface, not a specific CSV loader
    void run(qr_core::IDataProvider& data_source);
    double getPortfolioValue() {
        return portfolio_value_;
    }
    double getInitalCapital() {
        return initial_capital_;
    }
    double getCurrentCash(){
        return current_cash_;
    }

private:
    // Logic internal to the engine
    void on_tick(const qr_core::MarketData& tick);
    void check_signals(const qr_core::MarketData& tick, double spread, double entry_threshold);
    
    // Portfolio State
    double initial_capital_ = 100000.0; // Start with $100k
    double current_cash_ = 100000.0;
    double portfolio_value_ = 100000.0;
    double risk_per_trade_ = 0.10;

    // pairs state
    double current_spread = 0.0;
    
    // Position State
    bool is_in_position_ = false;
    bool is_short_ = false;
    double entry_spread_ = 0.0;
    double position_units_ = 0.0;
};

} 

#endif