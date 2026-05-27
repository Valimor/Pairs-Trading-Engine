#include <quant/engines/walk_forward_controller.hpp>
#include <quant/core/signal_policy.hpp>
#include <quant/engines/trading_engine.hpp>
#include <quant/core/math_utils.hpp>
#include <iostream>
#include <cmath>

namespace qr_engine {

// ============================================================================
// 1. Constructor Alignment Fix
// ============================================================================
WalkForwardController::WalkForwardController(qr_core::IDataProvider& provider, qr_core::TradeLogger& logger)
    : provider_(provider), 
      logger_(logger) 
{
}

// ============================================================================
// 2. Historical Sub-Matrix Data Extraction Methods
// ============================================================================
Eigen::VectorXd WalkForwardController::extract_historical_spreads(size_t start_idx, size_t end_idx) {
    size_t window_size = end_idx - start_idx;
    Eigen::VectorXd spreads(window_size);
    
    for (size_t i = 0; i < window_size; ++i) {
        auto tick = provider_.get_tick_at(start_idx + i);
        if (tick.has_value()) {
            // Compute rolling spread from pricing matrix components
            spreads(i) = qr_math::basic::calculate_spread(tick->price_a, tick->price_b, tick->beta);
        } else {
            spreads(i) = 0.0;
        }
    }
    return spreads;
}

Eigen::VectorXd WalkForwardController::extract_historical_vix(size_t start_idx, size_t end_idx) {
    size_t window_size = end_idx - start_idx;
    Eigen::VectorXd vix_vector(window_size);
    
    for (size_t i = 0; i < window_size; ++i) {
        auto tick = provider_.get_tick_at(start_idx + i);
        if (tick.has_value()) {
            vix_vector(i) = tick->vix; // Capturing exogenous shock variable
        } else {
            vix_vector(i) = 0.0;
        }
    }
    return vix_vector;
}

// ============================================================================
// 3. Concurrent/Iterative Grid Search Strategy
// ============================================================================
std::vector<qr_core::BacktestResult> WalkForwardController::run_parallel_search(size_t start_idx, size_t end_idx) {
    std::vector<qr_core::BacktestResult> global_search_results;

    Eigen::VectorXd sample_spreads = extract_historical_spreads(start_idx, end_idx);
    Eigen::VectorXd sample_vix     = extract_historical_vix(start_idx, end_idx);

    // Run the primary mathematical MLE fit to bind model parameters
    qr_math::GarchCalibrator calibrator;
    qr_math::GarchParameters calibrated_model = calibrator.fit(sample_spreads, sample_vix);

    // Establish hyperparameter search boundaries for your thresholds
    std::vector<double> entry_z_space  = { 1.5, 1.8, 2.0, 2.2, 2.5 };
    std::vector<double> stop_loss_space = { 0.02, 0.03, 0.05, 0.07 };

    // Loop through our threshold parameters grid (Can be parallelized via OpenMP)
    for (double current_z : entry_z_space) {
        for (double current_stop : stop_loss_space) {
            
            // Evaluate the specific thresholds using our cleanly fitted GARCH equations
            qr_core::BacktestResult run_metrics = evaluate_combination(
                start_idx, 
                end_idx, 
                current_z, 
                current_stop, 
                calibrated_model
            );

            // Save results if the backtest produced valid trading signals
            if (run_metrics.trade_count > 0) {
                global_search_results.push_back(run_metrics);
            }
        }
    }

    return global_search_results;
}

// ============================================================================
// 4. In-Sample Combination Evaluator Loop
// ============================================================================
qr_core::BacktestResult WalkForwardController::evaluate_combination(
    size_t train_start, 
    size_t train_end, 
    double entry_z, 
    double stop_loss, 
    const qr_math::GarchParameters& garch_params) 
{
    // Use an unmapped/quiet log profile to suppress IO overhead during optimization
    qr_core::TradeLogger optimization_quiet_logger("data/backtest_logs/null.csv"); 

    // Instantiate policy passing all parameters down cleanly to prevent truncation
    qr_engine::GarchPolicy local_policy(
        entry_z, 
        garch_params.alpha, 
        garch_params.beta, 
        garch_params.omega, 
        garch_params.gamma
    );

    qr_engine::TradingEngine local_engine(local_policy, optimization_quiet_logger);
    
    // Execute trade loops and pass metrics back out
    qr_core::BacktestResult metrics = local_engine.run_optimized(provider_, train_start, train_end, entry_z, stop_loss);
    
    metrics.applied_entry_z = entry_z;
    metrics.applied_stop = stop_loss;

    return metrics;
}

// ============================================================================
// 5. Out-of-Sample Forward Execution
// ============================================================================
qr_core::BacktestResult WalkForwardController::execute_forward_window(
    size_t train_start,
    size_t train_end,
    size_t oos_start, 
    size_t oos_end, 
    double entry_z, 
    double stop_loss) 
{

    // 1. Extract historical data directly within C++ space
    Eigen::VectorXd training_spreads = extract_historical_spreads(train_start, train_end);
    Eigen::VectorXd training_vix     = extract_historical_vix(train_start, train_end);

    // 2. Perform the MLE Fit locally on the stack
    qr_math::GarchCalibrator internal_calibrator;
    qr_math::GarchParameters internal_params = internal_calibrator.fit(training_spreads, training_vix);

    std::cout << "[OOS] Calibration complete. Generated Alpha: " << internal_params.alpha 
              << ", Beta: " << internal_params.beta << "\n";

    // 3. Construct the strategy policy using the freshly minted internal parameters
    qr_engine::GarchPolicy forward_policy(
        entry_z, 
        internal_params.alpha, 
        internal_params.beta, 
        internal_params.omega, 
        internal_params.gamma
    );

    // 4. Instantiate the trading engine and execute out-of-sample forward simulation
    qr_engine::TradingEngine forward_engine(forward_policy, logger_);
    
    qr_core::BacktestResult final_performance = forward_engine.run_optimized(
        provider_, 
        oos_start, 
        oos_end, 
        entry_z, 
        stop_loss
    );
    
    // Bind applied tracking hyperparameters to metadata fields
    final_performance.applied_entry_z = entry_z;
    final_performance.applied_stop = stop_loss;

    return final_performance;
}

} // namespace qr_engine