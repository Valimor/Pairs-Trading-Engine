#ifndef WALK_FORWARD_CONTROLLER_HPP
#define WALK_FORWARD_CONTROLLER_HPP

#include <quant/core/data_provider.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/core/garch_calibrator.hpp>
#include <quant/core/types.hpp>
#include <Eigen/Core>
#include <vector>
#include <future>

namespace qr_engine {

class WalkForwardController {
private:
    qr_core::IDataProvider& provider_;
    qr_core::TradeLogger& logger_; 


public:
    WalkForwardController(qr_core::IDataProvider& provider, qr_core::TradeLogger& logger);

    // 2. EXTRACTION UTILITIES
    Eigen::VectorXd extract_historical_spreads(size_t start_idx, size_t end_idx);
    Eigen::VectorXd extract_historical_vix(size_t start_idx, size_t end_idx);

    // 3. SEARCH ENGINE PATH
    std::vector<qr_core::BacktestResult> run_parallel_search(size_t start_idx, size_t end_idx);

    // Executes the chosen parameters strictly Out-of-Sample on a forward window
    qr_core::BacktestResult execute_forward_window(
        size_t train_start,
        size_t train_end,
        size_t oos_start, 
        size_t oos_end, 
        double entry_z, 
        double stop_loss
    );

    qr_core::BacktestResult evaluate_combination(
        size_t train_start, 
        size_t train_end, 
        double entry_z, 
        double stop_loss,
        const qr_math::GarchParameters& garch_params
    );
};

} // namespace qr_engine

#endif // WALK_FORWARD_CONTROLLER_HPP