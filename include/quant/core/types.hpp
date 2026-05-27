#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <string>
#include <fstream>
#include <cstdint>

namespace qr_core {
    struct Trade {
        std::string entryDate;
        std::string exitDate;
        double pnl;
    };

    struct MarketData {
        // Basic identifiers
        std::string symbol_a;
        std::string symbol_b;
        std::string date_str; // Or use a long long for unix epoch time
        std::uint64_t timestamp = 0; // The numerical version of the date
        
        // Price data
        double price_a;
        double price_b;
        
        // Derived or metadata fields
        double beta; 
        double vix;
        double avg_vix;

        // Derived (Optional to fill here or in engine)
        double spread;
        double z_score;
        
        // Volume (optional, but good for liquidity checks)
        uint64_t volume_a;
        uint64_t volume_b;
    };

    struct OptimizationResult {
        double alpha;
        double beta;
        double omega;
        double gamma;
        double total_pnl;
        double sharpe_ratio;
        double entry_z;
        double stop_loss;
        int trade_count;
    };

    struct BacktestResult {
        double total_pnl       = 0.0;
        double sharpe_ratio     = 0.0;
        double max_drawdown    = 0.0;
        int trade_count        = 0;
        
        // Contextual tracking tags
        double applied_entry_z = 0.0;
        double applied_stop    = 0.0;
    };
}
#endif