#ifndef WALK_FORWARD_CONTROLLER_HPP
#define WALK_FORWARD_CONTROLLER_HPP

#include <quant/core/data_provider.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/core/signal_policy.hpp>
#include <quant/engines/trading_engine.hpp>
#include <quant/core/summary_logger.hpp>
#include <quant/core/garch_calibrator.hpp>

#include <Eigen/Core>
#include <future>
#include <thread>

// include/quant/engines/walk_forward_controller.hpp
namespace qr_engine {

class WalkForwardController {
private:
    qr_core::IDataProvider& provider_;
    qr_core::TradeLogger& logger_; // Shared logger or one per window

public:
    WalkForwardController(qr_core::IDataProvider& p, qr_core::TradeLogger& l) 
        : provider_(p), logger_(l) {}

        qr_core::OptimizationResult execute_window_optimized(size_t start, size_t end, 
                                                         double entry_z, double stop_loss, 
                                                         const qr_core::OptimizationResult& best_config) 
    {
        // 1. Reconstruct the historical Garch configuration found during grid search
        qr_engine::GarchPolicy live_policy(best_config.alpha, best_config.beta, best_config.omega);
        
        // 2. Instantiate the engine bound to our system-level production logger
        qr_engine::TradingEngine production_engine(live_policy, logger_);

        // 3. Execute the trading strategy across the un-encountered out-of-sample forward window
        production_engine.run_optimized(provider_, start, end, entry_z, stop_loss);

        // 4. Package structural data back to your main analytical reporting layer
        qr_core::OptimizationResult res;
        res.alpha = best_config.alpha;
        res.beta = best_config.beta;
        res.entry_z = entry_z;
        res.stop_loss = stop_loss;
        res.total_pnl = production_engine.get_final_pnl();
        res.sharpe_ratio = production_engine.get_sharpe();
        res.trade_count = static_cast<int>(production_engine.get_trade_pnls().size());

        return res;
    }

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
            res.total_pnl = temp_engine.getPortfolioValue() - temp_engine.getInitialCapital();

            // Get the vector of PnLs from the engine
            auto pnls = temp_engine.get_trade_pnls(); 
            res.trade_count = static_cast<int>(pnls.size());
            res.sharpe_ratio = qr_math::performance::calculate_sharpe(pnls, temp_engine.getInitialCapital());
            
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

        std::vector<qr_core::OptimizationResult> run_parallel_search(size_t train_start, size_t train_end) {
        // 1. Run the Eigen MLE calibration ONCE on the main thread
        Eigen::VectorXd history = extract_historical_spreads(train_start, train_end);
        qr_math::GarchCalibrator calibrator;
        qr_math::GarchParameters calibrated_garch = calibrator.fit(history);

        // 2. Hardware Concurrency Layout Setup
        unsigned int max_cores = std::thread::hardware_concurrency();
        if (max_cores == 0) max_cores = 4; // Fallback default

        // FIXED: Move declaration to the top so loops can safely access it
        std::vector<qr_core::OptimizationResult> search_results;
        std::vector<std::future<qr_core::OptimizationResult>> futures;

        // 3. MAP: Populate tasks asynchronously across strategy thresholds
        for (double entry_z = 1.0; entry_z <= 3.0; entry_z += 0.5) {
            for (double stop_loss = 0.02; stop_loss <= 0.10; stop_loss += 0.02) {
                
                futures.push_back(std::async(std::launch::async, 
                    &WalkForwardController::evaluate_combination, this, 
                    train_start, train_end, entry_z, stop_loss, calibrated_garch
                ));

                // If we have filled up our physical CPU cores, wait for this batch to complete
                if (futures.size() >= max_cores) {
                    for (auto& fut : futures) {
                        search_results.push_back(fut.get()); // Blocks cleanly per batch
                    }
                    futures.clear(); // Clear the bucket for the next batch of cores
                }
            }
        }

        // 4. REDUCE: Gather any remaining leftover tasks from the final partial batch
        for (auto& fut : futures) {
            search_results.push_back(fut.get());
        }

        return search_results;
    }

        qr_core::OptimizationResult evaluate_combination(
        size_t start, size_t end, 
        double entry_z, double stop_loss, 
        const qr_math::GarchParameters& garch) 
    {
        // 1. Thread Isolation: Create a local, temporary logger for this specific thread.
        // This prevents multiple threads from trying to write to the main log file simultaneously.
        qr_core::TradeLogger thread_local_logger("C:/Users/maxim/Documents/GitHub/QR Projects/Pairs Trading Engine/data/backtest_logs/output.csv"); 

        // 2. Instantiate the engine passing the local logger and your policy interface
        // Note: If 'provider_' is a pointer in your class, pass *provider_
        qr_engine::GarchPolicy local_policy_(garch.alpha, garch.beta, garch.omega);
        qr_engine::TradingEngine local_engine(local_policy_, thread_local_logger); 
        
        // 3. RUN SIMULATION: Direct parameter injection bypasses the need for setters.
        // The GARCH alpha/beta values are passed into your metadata result struct, 
        // while the entry parameters control execution directly.
        local_engine.run_optimized(provider_, start, end, entry_z, stop_loss);

        // 4. Package performance metrics from the completed run
        qr_core::OptimizationResult res;
        res.alpha = garch.alpha;
        res.beta = garch.beta;
        res.entry_z = entry_z;
        res.stop_loss = stop_loss;
        res.sharpe_ratio = local_engine.get_sharpe();
        res.total_pnl = local_engine.get_final_pnl();

        return res;
    }

};
}

#endif