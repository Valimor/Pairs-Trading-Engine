#include <iostream>
#include <filesystem>
#include <quant/core/types.hpp>
#include <quant/core/csv_data_provider.hpp>
#include <quant/engines/walk_forward_controller.hpp>
#include <quant/core/summary_logger.hpp>
#include <algorithm>

#include "engines/trading_engine.cpp"


int main(int argc, char* argv[]) {
    // parse the given filepath
    // Check if a filename was provided as a command-line argument. if not, throw an error
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_csv_file>" << std::endl;
        return 1;
    }

    // Extract the path from the arguments
    std::string csvPath(argv[1]);

    // Choose the Data Source
    qr_core::CSVDataProvider provider(csvPath);
    provider.load_all();

    // choose number of ticks
    size_t total_size = provider.total_ticks();

    if (total_size < 10) { 
        std::cerr << "Error: Not enough data points in the provider to split into train/test sets.\n";
        return 1;
    }

    double train_ratio = 0.6;
    size_t train_ticks = static_cast<size_t>(total_size * train_ratio);
    size_t test_ticks = total_size;

    if (train_ticks < 5) {
        std::cerr << "Error: Calculated training window is too small for GARCH calibration.\n";
        return 1;
    }

    // initialize trades and log
    qr_core::TradeLogger test_logger("./data/backtest_logs/output.csv");
    qr_engine::WalkForwardController controller(provider, test_logger);
    qr_core::OptimizationResult result;

    // initial call
    result = controller.execute_window(0, train_ticks, 0.8, 0.05);

    //now, run the engine on the next data
    qr_core::OptimizationResult future_result = controller.execute_window(train_ticks, test_ticks, result.alpha, result.beta);

    std::cout  << future_result.total_pnl << "," << result.alpha 
            << "," << result.beta;

    return 0;
}