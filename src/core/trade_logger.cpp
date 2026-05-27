#include <quant/core/trade_logger.hpp>
#include <iostream>

namespace qr_core {

TradeLogger::TradeLogger(const std::string& filename) {
    file_.open(filename);
    if (!file_.is_open()) {
        std::cerr << "Error: Could not open trade log file: " << filename << std::endl;
        return;
    }
    file_ << "EntryDate,ExitDate,Side,EntryPrice,ExitPrice,PnL,TotalValue\n";
}

TradeLogger::~TradeLogger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void TradeLogger::log_trade(const TradeRecord& record) {
    if (file_.is_open()) {
        file_ << record.entry_date << "," << record.exit_date << "," 
              << record.side << "," << record.entry_price << "," 
              << record.exit_price << "," << record.pnl << "," 
              << record.final_portfolio_value << "\n";
        file_.flush(); 
    }
}

} // namespace qr_core