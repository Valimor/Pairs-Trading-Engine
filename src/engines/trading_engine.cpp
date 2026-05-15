#include <quant/engines/trading_engine.hpp>
#include <quant/core/math_utils.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/core/signal_policy.hpp>
#include <iostream>

namespace qr_engine {

void TradingEngine::run(qr_core::IDataProvider& provider, size_t start_idx, size_t end_idx) {
    // Reset state for a fresh window run
    reset_portfolio(); 

    for (size_t i = start_idx; i < end_idx; ++i) {
        auto tick = provider.get_tick_at(i);
        if (tick.has_value()) {
            on_tick(tick.value());
        }
    }
}

void TradingEngine::on_tick(const qr_core::MarketData& tick) {
    double current_spread = qr_math::basic::calculate_spread(tick.price_a, tick.price_b, tick.beta);
    
    // The engine asks its assigned strategy for the threshold
    double dynamic_entry = policy_.get_threshold(tick);
    
    // Matches your required signature perfectly
    check_signals(tick, current_spread, dynamic_entry);
}

void TradingEngine::check_signals(const qr_core::MarketData& tick, double spread, double entry_threshold) {
    double z = tick.z_score;

    if (!is_in_position_) {
        // ENTRY LOGIC
        if (std::abs(z) > entry_threshold) {
            is_in_position_ = true;
            is_short_ = (z > 0);
            entry_spread_ = spread;

            // Calculate sizing
            double capital_to_deploy = portfolio_value_ * risk_per_trade_;
            position_units_ = capital_to_deploy / std::abs(spread);
            current_cash_ -= capital_to_deploy;

            // Fill the 'Entry' portion of our persistent record
            current_trade_record_.entry_date = tick.date_str;
            current_trade_record_.entry_price = spread;
            current_trade_record_.side = is_short_ ? "SHORT" : "LONG";
        }
    } 
    else {
        // EXIT LOGIC (Mean Reversion)
        if ((is_short_ && z <= 0) || (!is_short_ && z >= 0)) {
            
            double price_diff = is_short_ ? (entry_spread_ - spread) 
                                         : (spread - entry_spread_);
            
            double trade_pnl = price_diff * position_units_;

            trade_pnls_.push_back(trade_pnl);

            // Update Portfolio
            double capital_returned = (portfolio_value_ * risk_per_trade_) + trade_pnl;
            current_cash_ += capital_returned;
            portfolio_value_ = current_cash_;

            // Complete the 'Exit' portion of the record
            current_trade_record_.exit_date = tick.date_str;
            current_trade_record_.exit_price = spread;
            current_trade_record_.pnl = trade_pnl;
            current_trade_record_.final_portfolio_value = portfolio_value_;

            // Ship the record to the logger
            logger_.log_trade(current_trade_record_);

            // Reset Engine State (Independent of the Record)
            is_in_position_ = false;
            position_units_ = 0.0;
            entry_spread_ = 0.0;
        }
    }
}

} // namespace qr_engine