#ifndef SUMMARY_LOGGER_HPP
#define SUMMARY_LOGGER_HPP

#include <fstream>
#include <vector>
#include <string>

namespace qr_core {

struct OptimizationResult {
    double alpha;
    double beta;
    double omega;
    double total_pnl;
    double sharpe_ratio;
    double entry_z;
    double stop_loss;
    int trade_count;
};

class SummaryLogger {
private:
    std::string filename_;

public:
    SummaryLogger(const std::string& filename) : filename_(filename) {}

    void log_results(const std::vector<OptimizationResult>& results) {
        std::ofstream file(filename_);
        file << "Alpha,Beta,TotalPnL,SharpeRatio,TradeCount\n";
        
        for (const auto& res : results) {
            file << res.alpha << "," 
                 << res.beta << "," 
                 << res.total_pnl << "," 
                 << res.sharpe_ratio << "," 
                 << res.trade_count << "\n";
        }
    }
};

}
#endif