#ifndef TRADING_ENGINE_HPP
#define TRADING_ENGINE_HPP

#include <quant/core/data_provider.hpp>
#include <quant/core/types.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/core/signal_policy.hpp>
#include <memory>
#include <vector>
#include <cmath>
#include <numeric>

namespace qr_engine {

class TradingEngine {
public:
    // Constructor uses your existing dependencies
    TradingEngine(ISignalPolicy& policy, qr_core::TradeLogger& logger) 
        : policy_(policy), logger_(logger) {}

    // OVERLOAD FOR PARALLEL OPTIMIZATION: Allows overriding parameters per thread run
    void run_optimized(qr_core::IDataProvider& provider, size_t start_idx, size_t end_idx, 
                       double custom_entry_z, double custom_stop_loss);

    // Standard run interface
    void run(qr_core::IDataProvider& provider, size_t start_idx, size_t end_idx);
    
    // Performance metrics
    double getPortfolioValue() const { return portfolio_value_; }
    double getInitialCapital() const { return initial_capital_; }
    double getCurrentCash() const { return current_cash_; }
    const std::vector<double>& get_trade_pnls() const { return trade_pnls_; }
    
    // Calculated Performance Metrics for the Grid Search
    double get_final_pnl() const { return portfolio_value_ - initial_capital_; }
    double get_sharpe() const;

private:
    void on_tick(const qr_core::MarketData& tick, double entry_override, double stop_override);
    void check_signals(const qr_core::MarketData& tick, double spread, double entry_threshold, double stop_loss_pct);

    ISignalPolicy& policy_;
    qr_core::TradeLogger& logger_;
    qr_core::TradeRecord current_trade_record_;
    
    // Portfolio State
    double initial_capital_ = 100000.0;
    double current_cash_ = 100000.0;
    double portfolio_value_ = 100000.0;
    double risk_per_trade_ = 0.10;
    std::vector<double> trade_pnls_;

    // Position State
    bool is_in_position_ = false;
    bool is_short_ = false;
    double entry_spread_ = 0.0;
    double position_units_ = 0.0;

    void reset_portfolio() {
        current_cash_ = initial_capital_;
        portfolio_value_ = initial_capital_;
        trade_pnls_.clear();
        is_in_position_ = false;
        is_short_ = false;
        entry_spread_ = 0.0;
        position_units_ = 0.0;
    }
};

} // namespace qr_engine

#endif