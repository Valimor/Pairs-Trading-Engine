#ifndef CSV_DATA_PROVIDER_HPP
#define CSV_DATA_PROVIDER_HPP

#include <quant/core/data_provider.hpp>
#include <quant/core/types.hpp>
#include <vector>
#include <string>
#include <optional>

namespace qr_core {

class CSVDataProvider : public IDataProvider {
private:
    std::vector<MarketData> data_buffer;
    size_t current_index = 0;
    std::string file_path_;

    // Private analytical helpers
    std::time_t parse_date(const std::string& date_str);
    double safe_stod(const std::string& s);
    MarketData parse_line(const std::string& line);

public:
    explicit CSVDataProvider(const std::string& filepath);

    // Overridden IDataProvider Interface methods
    std::optional<MarketData> get_next_tick() override;
    std::optional<MarketData> get_tick_at(size_t index) override;
    size_t total_ticks() const override;
    void reset() override;

    // Mass ingestion processing
    void load_all();
};

} // namespace qr_core

#endif // CSV_DATA_PROVIDER_HPP