#include <iostream>
#include <filesystem>
#include <vector>
#include <iomanip>
#include <quant/core/types.hpp>
#include <quant/core/csv_data_provider.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/engines/walk_forward_controller.hpp>

int main(int argc, char* argv[]) {
    // 1. Command Line Argument Verification
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_csv_file>" << std::endl;
        return 1;
    }

    std::string csvPath(argv[1]);
    if (!std::filesystem::exists(csvPath)) {
        std::cerr << "Error: File does not exist at " << csvPath << std::endl;
        return 1;
    }

    // 2. Data Ingestion
    std::cout << "[INIT] Loading historical tick data from: " << csvPath << "\n";
    qr_core::CSVDataProvider provider(csvPath);
    provider.load_all();

    size_t total_size = provider.total_ticks();
    std::cout << "[DATA] Successfully loaded " << total_size << " total ticks.\n";

    // 3. Define Train/Test Structural Splits
    double train_ratio = 0.6;
    size_t train_start = 0;
    size_t train_end = static_cast<size_t>(total_size * train_ratio);
    size_t test_start = train_end;
    size_t test_end = total_size;

    // Safety constraints
    if (train_end < 10 || (test_end - test_start) < 5) { 
        std::cerr << "Error: Insufficient data bounds to construct validation splits.\n";
        return 1;
    }

    std::cout << "[SPlIT] In-Sample (Train) Range: " << train_start << " -> " << train_end << "\n";
    std::cout << "[SPLIT] Out-of-Sample (Test) Range: " << test_start << " -> " << test_end << "\n";

    // 4. Initialize Core Infrastructure Elements
    qr_core::TradeLogger live_logger("./data/backtest_logs/execution_run.csv");
    qr_engine::WalkForwardController controller(provider, live_logger);

    // 5. PHASE 1: Run the Concurrent Grid Search over the In-Sample Window
    std::cout << "[PHASE 1] Executing concurrent grid search over In-Sample region...\n";
    std::vector<qr_core::OptimizationResult> search_results = controller.run_parallel_search(train_start, train_end);

    if (search_results.empty()) {
        std::cerr << "Error: Grid search returned zero valid parameter results.\n";
        return 1;
    }

    // 6. PHASE 2: Identify the Optimal Hyperparameter Set (Maximize Sharpe Ratio)
    qr_core::OptimizationResult best_config = search_results[0];
    for (const auto& res : search_results) {
        if (res.sharpe_ratio > best_config.sharpe_ratio) {
            best_config = res;
        }
    }

    std::cout << "\n========================================================\n";
    std::cout << "          OPTIMAL IN-SAMPLE HYPERPARAMETERS FOUND       \n";
    std::cout << "========================================================\n";
    std::cout << "Calibrated GARCH Alpha : " << best_config.alpha << "\n";
    std::cout << "Calibrated GARCH Beta  : " << best_config.beta << "\n";
    std::cout << "Optimal Entry Z-Score  : " << best_config.entry_z << "\n";
    std::cout << "Optimal Stop-Loss Pct  : " << (best_config.stop_loss * 100.0) << "%\n";
    std::cout << "In-Sample Sharpe Ratio : " << best_config.sharpe_ratio << "\n";
    std::cout << "In-Sample Backtest PnL : $" << best_config.total_pnl << "\n";
    std::cout << "========================================================\n\n";

    // 7. PHASE 3: Out-of-Sample Forward Validation 
    // We lock the optimized parameters and pass them directly into the unseen forward testing window.
    std::cout << "[PHASE 3] Running out-of-sample verification step...\n";
    
    // Ensure your WalkForwardController has an implementation of execute_window 
    // that routes to engine.run_optimized using these parameters
    qr_core::OptimizationResult forward_validation = controller.execute_window_optimized(
        test_start, 
        test_end, 
        best_config.entry_z, 
        best_config.stop_loss,
        best_config // Pass structural GARCH coefficients
    );

    // 8. Performance Metrics Analysis Output
    std::cout << "\n========================================================\n";
    std::cout << "          FORWARD TESTING (OUT-OF-SAMPLE) RESULTS       \n";
    std::cout << "========================================================\n";
    std::cout << "Out-of-Sample PnL      : $" << forward_validation.total_pnl << "\n";
    std::cout << "Out-of-Sample Sharpe   : " << forward_validation.sharpe_ratio << "\n";
    
    // Performance degradation tracking checks for model overfitting
    double performance_decay = best_config.sharpe_ratio - forward_validation.sharpe_ratio;
    std::cout << "Sharpe Variance (Decay): " << performance_decay << "\n";
    std::cout << "========================================================\n";

    return 0;
}