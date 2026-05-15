#ifndef WALK_FORWARD_CONTROLLER_HPP
#define WALK_FORWARD_CONTROLLER_HPP

#include <quant/core/data_provider.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/core/signal_policy.hpp>
#include <quant/engines/trading_engine.hpp>
#include <quant/core/summary_logger.hpp>
#include <quant/core/garch_calibrator.hpp>

#include <Eigen/Core>

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

        qr_math::GarchCalibrator calibrator;
        Eigen::VectorXd history = extract_historical_spreads(start, end);
        qr_math::GarchParameters calibrated_params = calibrator.fit(history);

        // Collect the results
        qr_core::OptimizationResult res;
        res.alpha = calibrated_params.alpha;
        res.beta = calibrated_params.beta;
        res.total_pnl = temp_engine.getPortfolioValue() - temp_engine.getInitalCapital();

        // Get the vector of PnLs from the engine
        auto pnls = temp_engine.get_trade_pnls(); 
        res.trade_count = static_cast<int>(pnls.size());
        res.sharpe_ratio = qr_math::performance::calculate_sharpe(pnls, temp_engine.getInitalCapital());
        
        return res;
    }

    Eigen::VectorXd extract_historical_spreads(size_t start_idx, size_t end_idx) {
        // CRITICAL GUARD: Catch an invalid or negative range before Eigen allocates
        if (end_idx <= start_idx) {
            std::cerr << "CRITICAL: extract_historical_spreads called with invalid bounds! "
                    << "start_idx: " << start_idx << ", end_idx: " << end_idx << "\n";
            return Eigen::VectorXd::Zero(0); // Return empty, which our Garch guard will catch
        }

        size_t window_size = end_idx - start_idx;
        Eigen::VectorXd spreads(window_size); // This is where the line 300 crash happens!

        for (size_t i = 0; i < window_size; ++i) {
            auto tick = provider_.get_tick_at(start_idx + i);
            if (tick) {
                spreads(i) = tick->z_score; 
            } else {
                spreads(i) = 0.0; // Handle missing ticks gracefully
            }
        }
        return spreads;
    }

};
}

#endif