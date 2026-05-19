#ifndef CSV_DATA_PROVIDER_HPP
#define CSV_DATA_PROVIDER_HPP

#include <quant/core/data_provider.hpp>
#include <quant/core/types.hpp>
#include <iomanip>

namespace qr_core {

class CSVDataProvider : public IDataProvider {
private:
    std::vector<MarketData> data_buffer;
    size_t current_index = 0;
    std::string file_path_;

    // Helper: Converts "YYYY-MM-DD" to a numerical timestamp
    std::time_t parse_date(const std::string& date_str) {
        std::tm t = {};
        std::istringstream ss(date_str);
        ss >> std::get_time(&t, "%Y-%m-%d");
        if (ss.fail()) return 0;
        return std::mktime(&t);
    }

    // Helper: Safely converts string to double, returning 0.0 on failure
    double safe_stod(const std::string& s) {
        if (s.empty()) return 0.0;
        try {
            return std::stod(s);
        } catch (...) {
            return 0.0; // Prevents crash on bad data
        }
    }

public:
    CSVDataProvider(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filepath << std::endl;
            return;
        }

        std::string line;
        std::getline(file, line); // Skip header

        while (std::getline(file, line)) {
            if (line.empty()) continue; // Skip empty lines at end of file

            data_buffer.push_back(parse_line(line));
        }
    }

    std::optional<MarketData> get_next_tick() override {
        if (current_index < data_buffer.size()) {
            return data_buffer[current_index++];
        }
        return std::nullopt;
    }

    MarketData parse_line(std::string line) {
        MarketData row;

        std::stringstream ss(line);
        std::string col;
        std::vector<std::string> cols;

        while (std::getline(ss, col, ',')) {
            cols.push_back(col);
        }

        row.date_str = cols[0];
        row.timestamp = parse_date(cols[0]);
        
        if (cols.size() >= 6) {
            row.price_a = safe_stod(cols[1]);
            row.price_b = safe_stod(cols[2]);
            row.vix     = safe_stod(cols[3]);
            row.avg_vix = safe_stod(cols[4]);
            row.z_score = safe_stod(cols[5]);
            //row.beta    = safe_stod(cols[6]);
        }

        return row;
    }

    void load_all() {
        std::ifstream file(file_path_);
        std::string line;
        
        // Skip header
        std::getline(file, line);

        while (std::getline(file, line)) {
            // Use your existing parsing logic (safe_stod, etc.)
            MarketData tick = parse_line(line); 
            data_buffer.push_back(tick);
        }
    }

    std::optional<MarketData> get_tick_at(size_t index) override {
        if(index < data_buffer.size()) {
            return data_buffer[index];
        }
        return std::nullopt;
    }

    size_t total_ticks() const override {
        return data_buffer.size();
    }

    void reset() override {
        current_index = 0;
    }
};

} // namespace qr_core

#endif