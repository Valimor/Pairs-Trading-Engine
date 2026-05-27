#ifndef TRADE_LOGGER_HPP
#define TRADE_LOGGER_HPP

#include <string>
#include <fstream>

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
    std::ofstream file_; // Renamed to style convention

public:
    explicit TradeLogger(const std::string& filename);
    ~TradeLogger(); // Explicitly close the file safely on destruction

    void log_trade(const TradeRecord& record);
};

} // namespace qr_core

#endif // TRADE_LOGGER_HPP