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

    // initialize trades and log
    qr_core::TradeLogger test_logger("./data/backtest_logs/output.csv");
    qr_engine::WalkForwardController controller(provider, test_logger);

    std::vector<qr_core::OptimizationResult> all_results;

    for (double a = 0.01; a <= 0.1; a += 0.02) {
        for (double b = 0.8; b <= 0.95; b += 0.05) {
            // Run a window from index 0 to 1000 with these params
            all_results.push_back(controller.execute_window(0, 1000, a, b));
        }
    }

    // Sort by pnl descending
    // could do sharpe ratio, but pnl is fun
    std::sort(all_results.begin(), all_results.end(), [](const qr_core::OptimizationResult& a, const qr_core::OptimizationResult& b) {
        return a.total_pnl > b.total_pnl;
    });

    // The "Winner" is now at all_results[0]
    auto best = all_results[0];
    std::cout << "Best Alpha: " << best.alpha << " | Best Beta: " << best.beta << std::endl;

    //log it!
    qr_core::SummaryLogger sum_log("./data/backtest_logs/grid_search_summary.csv");
    sum_log.log_results(all_results);

    return 0;
}