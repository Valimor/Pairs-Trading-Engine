#include <iostream>
#include <filesystem>
#include <vector>
#include <iomanip>
#include <quant/core/types.hpp>
#include <quant/core/csv_data_provider.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/core/garch_calibrator.hpp> // Added for GarchCalibrator
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

    std::cout << "[SPLIT] In-Sample (Train) Range: " << train_start << " -> " << train_end << "\n";
    std::cout << "[SPLIT] Out-of-Sample (Test) Range: " << test_start << " -> " << test_end << "\n";

    // 4. Initialize Core Infrastructure Elements
    qr_core::TradeLogger live_logger("./data/backtest_logs/execution_run.csv");
    qr_engine::WalkForwardController controller(provider, live_logger);

    // ============================================================================
    // NEW: HISTORICAL GARCH-X MAXIMUM LIKELIHOOD ESTIMATION (MLE) FIT
    // ============================================================================
    std::cout << "[FIT] Extracting in-sample historical matrices for calibration...\n";
    Eigen::VectorXd in_sample_spreads = controller.extract_historical_spreads(train_start, train_end);
    Eigen::VectorXd in_sample_vix     = controller.extract_historical_vix(train_start, train_end);

    std::cout << "[FIT] Executing GARCH-X MLE optimization loop...\n";
    qr_math::GarchCalibrator calibrator;
    qr_math::GarchParameters fitted_garch = calibrator.fit(in_sample_spreads, in_sample_vix);

    std::cout << "\n========================================================\n";
    std::cout << "             GARCH-X MODEL CALIBRATION COMPLETION        \n";
    std::cout << "========================================================\n";
    std::cout << "Fitted Omega (Baseline Var) : " << fitted_garch.omega << "\n";
    std::cout << "Fitted Alpha (Shock Impact) : " << fitted_garch.alpha << "\n";
    std::cout << "Fitted Beta (Persistence)   : " << fitted_garch.beta << "\n";
    std::cout << "Fitted Gamma (Exogenous VIX): " << fitted_garch.gamma << "\n";
    std::cout << "Log-Likelihood Target Score : " << fitted_garch.nll << "\n";
    std::cout << "========================================================\n\n";

    // 5. PHASE 1: Run the Concurrent Grid Search over the In-Sample Window
    std::cout << "[PHASE 1] Executing concurrent grid search over In-Sample region...\n";
    // FIXED: Changed type from OptimizationResult to BacktestResult
    std::vector<qr_core::BacktestResult> search_results = controller.run_parallel_search(train_start, train_end);

    if (search_results.empty()) {
        std::cerr << "Error: Grid search returned zero valid parameter results.\n";
        return 1;
    }

    // 6. PHASE 2: Identify the Optimal Hyperparameter Set (Maximize Sharpe Ratio)
    qr_core::BacktestResult best_config = search_results[0];
    for (const auto& res : search_results) {
        if (res.sharpe_ratio > best_config.sharpe_ratio) {
            best_config = res;
        }
    }

    std::cout << "\n========================================================\n";
    std::cout << "          OPTIMAL IN-SAMPLE HYPERPARAMETERS FOUND       \n";
    std::cout << "========================================================\n";
    std::cout << "Calibrated GARCH Alpha : " << fitted_garch.alpha << "\n";
    std::cout << "Calibrated GARCH Beta  : " << fitted_garch.beta << "\n";
    std::cout << "Calibrated GARCH Gamma : " << fitted_garch.gamma << "\n";
    std::cout << "Optimal Entry Z-Score  : " << best_config.applied_entry_z << "\n";
    std::cout << "Optimal Stop-Loss Pct  : " << (best_config.applied_stop * 100.0) << "%\n";
    std::cout << "In-Sample Sharpe Ratio : " << best_config.sharpe_ratio << "\n";
    std::cout << "In-Sample Backtest PnL : $" << best_config.total_pnl << "\n";
    std::cout << "========================================================\n\n";

    // 7. PHASE 3: Out-of-Sample Forward Validation 
    std::cout << "[PHASE 3] Running out-of-sample verification step...\n";
    
    // FIXED: Catches performance fields using the correct BacktestResult definition 
    qr_core::BacktestResult forward_validation = controller.execute_forward_window(
        train_start,
        train_end,
        test_start, 
        test_end, 
        best_config.applied_entry_z, 
        best_config.applied_stop
    );

    // 8. Performance Metrics Analysis Output
    std::cout << "\n========================================================\n";
    std::cout << "          FORWARD TESTING (OUT-OF-SAMPLE) RESULTS       \n";
    std::cout << "========================================================\n";
    std::cout << "Out-of-Sample PnL      : $" << forward_validation.total_pnl << "\n";
    std::cout << "Out-of-Sample Sharpe   : " << forward_validation.sharpe_ratio << "\n";
    
    double performance_decay = best_config.sharpe_ratio - forward_validation.sharpe_ratio;
    std::cout << "Sharpe Variance (Decay): " << performance_decay << "\n";
    std::cout << "========================================================\n";

    return 0;
}