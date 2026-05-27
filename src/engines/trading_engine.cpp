#include <quant/engines/trading_engine.hpp>
#include <quant/core/math_utils.hpp>
#include <iostream>
#include <numeric>
#include <cmath>

namespace qr_engine {

// ============================================================================
// Standard Path (Using policy's internal/calibrated parameters)
// ============================================================================
qr_core::BacktestResult TradingEngine::run(qr_core::IDataProvider& provider, size_t start_idx, size_t end_idx) {
    reset_portfolio(); 
    
    for (size_t i = start_idx; i < end_idx; ++i) {
        auto tick = provider.get_tick_at(i);
        if (tick.has_value()) {
            // Passing negative sentinels to tell the engine to use policy thresholds
            on_tick(tick.value(), -1.0, -1.0); 
        }
    }

    // Pack and return performance summaries
    qr_core::BacktestResult results;
    results.total_pnl = portfolio_value_ - 100000.0; // TODO: CHANGE THIS 100000 to not hardcoded.
    results.trade_count = static_cast<int>(trade_pnls_.size());
    results.sharpe_ratio = get_sharpe();
    results.applied_entry_z = -1.0; // Managed dynamically by policy engine
    results.applied_stop = 0.05;    // Default base fallback
    
    return results;
}

// ============================================================================
// Optimization/Grid Path (Forcing explicit parameters per core run)
// ============================================================================
qr_core::BacktestResult TradingEngine::run_optimized(qr_core::IDataProvider& provider, size_t start_idx, size_t end_idx, 
                                                   double custom_entry_z, double custom_stop_loss) {
    reset_portfolio();
    
    for (size_t i = start_idx; i < end_idx; ++i) {
        auto tick = provider.get_tick_at(i);
        if (tick.has_value()) {
            on_tick(tick.value(), custom_entry_z, custom_stop_loss);
        }
    }

    // Pack and return parameters directly to the grid search loop
    qr_core::BacktestResult results;
    results.total_pnl = portfolio_value_ - 100000.0; 
    results.trade_count = static_cast<int>(trade_pnls_.size());
    results.sharpe_ratio = get_sharpe();
    results.applied_entry_z = custom_entry_z;
    results.applied_stop = custom_stop_loss;
    
    return results;
}

// ============================================================================
// Tick Processing Core Engine
// ============================================================================
void TradingEngine::on_tick(const qr_core::MarketData& tick, double entry_override, double stop_override) {
    double current_spread = qr_math::basic::calculate_spread(tick.price_a, tick.price_b, tick.beta);
    
    // Check for parameter overrides. Otherwise consult the policy object
    double entry_threshold = (entry_override > 0.0) ? entry_override : policy_.get_threshold(tick);
    double stop_loss = (stop_override > 0.0) ? stop_override : 0.05; 

    check_signals(tick, current_spread, entry_threshold, stop_loss);
}

void TradingEngine::check_signals(const qr_core::MarketData& tick, double spread, double entry_threshold, double stop_loss_pct) {
    double z = tick.z_score;

    if (!is_in_position_) {
        // --- ENTRY EXECUTION SIGNAL LOGIC ---
        if (std::abs(z) > entry_threshold) {
            is_in_position_ = true;
            is_short_ = (z > 0);
            entry_spread_ = spread;

            double capital_to_deploy = portfolio_value_ * risk_per_trade_;
            position_units_ = capital_to_deploy / std::abs(spread);
            current_cash_ -= capital_to_deploy;

            current_trade_record_.entry_date = tick.date_str;
            current_trade_record_.entry_price = spread;
            current_trade_record_.side = is_short_ ? "SHORT" : "LONG";
        }
    } 
    else {
        // --- POSITION RISK ASSESSMENT ---
        double price_diff = is_short_ ? (entry_spread_ - spread) : (spread - entry_spread_);
        double running_pnl = price_diff * position_units_;
        double initial_allocation = portfolio_value_ * risk_per_trade_;

        // Risk Bounds Trigger Evaluation
        bool stop_loss_triggered = (running_pnl < -1.0 * (initial_allocation * stop_loss_pct));
        bool mean_reverted = ((is_short_ && z <= 0) || (!is_short_ && z >= 0));

        // --- EXIT EXECUTION ---
        if (mean_reverted || stop_loss_triggered) {
            trade_pnls_.push_back(running_pnl);

            double capital_returned = initial_allocation + running_pnl;
            current_cash_ += capital_returned;
            portfolio_value_ = current_cash_;

            current_trade_record_.exit_date = tick.date_str;
            current_trade_record_.exit_price = spread;
            current_trade_record_.pnl = running_pnl;
            current_trade_record_.final_portfolio_value = portfolio_value_;

            // Thread-safe persistence flush out to system disk
            logger_.log_trade(current_trade_record_);

            // State Reset Cleanups
            is_in_position_ = false;
            position_units_ = 0.0;
            entry_spread_ = 0.0;
        }
    }
}

// ============================================================================
// Performance Analytics Computations
// ============================================================================
double TradingEngine::get_sharpe() const {
    if (trade_pnls_.empty()) return 0.0;

    double sum = std::accumulate(trade_pnls_.begin(), trade_pnls_.end(), 0.0);
    double mean = sum / trade_pnls_.size();

    double sq_sum = 0.0;
    for (double pnl : trade_pnls_) {
        sq_sum += (pnl - mean) * (pnl - mean);
    }
    
    double variance = sq_sum / trade_pnls_.size();
    double stdev = std::sqrt(variance);

    if (stdev < 1e-6) return 0.0; 

    return mean / stdev;
}

} // namespace qr_engine