#include <quant/core/summary_logger.hpp>
#include <quant/core/types.hpp>
#include <fstream>
#include <iostream>

namespace qr_core {

SummaryLogger::SummaryLogger(const std::string& filename) : filename_(filename) {}

void SummaryLogger::log_results(const std::vector<OptimizationResult>& results) {
    std::ofstream file(filename_);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open summary log file: " << filename_ << std::endl;
        return;
    }

    file << "Alpha,Beta,TotalPnL,SharpeRatio,TradeCount\n";
    for (const auto& res : results) {
        file << res.alpha << "," 
             << res.beta << "," 
             << res.total_pnl << "," 
             << res.sharpe_ratio << "," 
             << res.trade_count << "\n";
    }
}

} // namespace qr_core