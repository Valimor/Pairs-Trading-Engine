#ifndef WALK_FORWARD_CONTROLLER_HPP
#define WALK_FORWARD_CONTROLLER_HPP

#include <quant/core/data_provider.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/core/signal_policy.hpp>
#include <quant/engines/trading_engine.hpp>
#include <quant/core/summary_logger.hpp>

// include/quant/engines/walk_forward_controller.hpp
namespace qr_engine {

class WalkForwardController {
private:
    qr_core::IDataProvider& provider_;
    qr_core::TradeLogger& logger_; // Shared logger or one per window

    //values for tracking the best performance

public:
    WalkForwardController(qr_core::IDataProvider& p, qr_core::TradeLogger& l) 
        : provider_(p), logger_(l) {}

    qr_core::OptimizationResult execute_window(size_t start, size_t end, double a, double b) {
        // Factory: Create a temporary engine for this specific test
        GarchPolicy temp_policy(2.0, a, b);
        TradingEngine temp_engine(temp_policy, logger_);

        // Execute only on the slice defined by the controller
        temp_engine.run(provider_, start, end);

        std::cout << temp_engine.getPortfolioValue() - temp_engine.getInitalCapital() << "\n";

        // Collect the results
        qr_core::OptimizationResult res;
        res.alpha = a;
        res.beta = b;
        res.total_pnl = temp_engine.getPortfolioValue() - temp_engine.getInitalCapital();

        // Get the vector of PnLs from the engine
        auto pnls = temp_engine.get_trade_pnls(); 
        res.trade_count = static_cast<int>(pnls.size());
        res.sharpe_ratio = qr_math::performance::calculate_sharpe(pnls, temp_engine.getInitalCapital());
        
        return res;
    }
};

}

#endif