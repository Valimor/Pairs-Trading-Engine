#include <quant/engines/trading_engine.hpp>
#include <quant/core/math_utils.hpp>
#include <iostream>

namespace qr_engine {

void TradingEngine::run(qr_core::IDataProvider& data_source) {
    
    while (auto tick = data_source.get_next_tick()) {
        on_tick(tick.value());
    }

    // 4. return data
    std::cout << portfolio_value_ - initial_capital_ << "\n";
}

void TradingEngine::on_tick(const qr_core::MarketData& tick) {
    // 1. Calculate the current spread using the beta from the data source
    double current_spread = qr_math::basic::calculate_spread(tick.price_a, tick.price_b, tick.beta);
    
    // 2. Adjust thresholds based on VIX (Dynamic Thresholding)
    double base_entry = 2.0; // Your classic 2.0 Z-Score entry
    double dynamic_entry = qr_math::signals::get_vol_adjusted_threshold(base_entry, tick.vix, tick.avg_vix);
    
    // 3. Signal Generation Logic
    check_signals(tick, current_spread, dynamic_entry);
}

void TradingEngine::check_signals(const qr_core::MarketData& tick, double current_spread, double entry_threshold) {
    
    // Safety Check: If spread is effectively zero, skip this tick to avoid NaN
    if (std::abs(current_spread) < 0.000001) return;

    double z = tick.z_score;

    //review this logic!
    if (!is_in_position_) {
        if (std::abs(z) > entry_threshold) {
            is_in_position_ = true;
            is_short_ = (z > 0);
            entry_spread_ = current_spread;

            double capital_to_deploy = portfolio_value_ * risk_per_trade_;
            
            // NaN-Proofing: Ensure we don't divide by zero
            position_units_ = capital_to_deploy / std::abs(current_spread);
            
            current_cash_ -= capital_to_deploy; 
        }
    } 
    else {
        // EXIT
        double price_diff = is_short_ ? (entry_spread_ - current_spread) 
                                     : (current_spread - entry_spread_);
        
        double trade_pnl = price_diff * position_units_;

        // Safety Check: If something went wrong, don't update portfolio with NaN
        if (std::isnan(trade_pnl) || std::isinf(trade_pnl)) {
            std::cerr << "Warning: Trade PnL calculated as NaN/Inf. Skipping update." << std::endl;
        } else {
            current_cash_ += (portfolio_value_ * risk_per_trade_) + trade_pnl;
            portfolio_value_ = current_cash_;
        }
        
        is_in_position_ = false;
    }
}

} // namespace qr_engine