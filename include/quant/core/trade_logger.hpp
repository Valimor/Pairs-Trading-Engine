#ifndef TRADE_LOGGER_HPP
#define TRADE_LOGGER_HPP

#include <fstream>
#include <string>

namespace qr_core {

struct TradeRecord {
    std::string entry_date;
    std::string exit_date;
    std::string side;
    double entry_price;
    double exit_price;
    double pnl;
    double final_portfolio_value;
};

class TradeLogger {
private:
    std::ofstream file;

public:
    TradeLogger(const std::string& filename) {
        file.open(filename);
        file << "EntryDate,ExitDate,Side,EntryPrice,ExitPrice,PnL,TotalValue\n";
    }

    void log_trade(const TradeRecord& record) {
        if (file.is_open()) {
            file << record.entry_date << "," << record.exit_date << "," 
                 << record.side << "," << record.entry_price << "," 
                 << record.exit_price << "," << record.pnl << "," 
                 << record.final_portfolio_value << "\n";
            file.flush(); // Ensure data is written even if program crashes
        }
    }
};

}
#endif