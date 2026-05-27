#include <quant/core/csv_data_provider.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace qr_core {



// Constructor: FIXED to properly initialize file_path_ using an initialization list
CSVDataProvider::CSVDataProvider(const std::string& filepath) 
    : current_index(0), file_path_(filepath) 
{
    std::ifstream file(file_path_);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << file_path_ << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // Skip CSV header row

    while (std::getline(file, line)) {
        if (line.empty()) continue; 
        data_buffer.push_back(parse_line(line));
    }
}

std::time_t CSVDataProvider::parse_date(const std::string& date_str) {
    std::tm t = {};
    std::istringstream ss(date_str);
    ss >> std::get_time(&t, "%Y-%m-%d");
    if (ss.fail()) return 0;
    return std::mktime(&t);
}

double CSVDataProvider::safe_stod(const std::string& s) {
    if (s.empty()) return 0.0;
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0; 
    }
}

MarketData CSVDataProvider::parse_line(const std::string& line) {
    MarketData row;
    std::stringstream ss(line);
    std::string col;
    std::vector<std::string> cols;

    while (std::getline(ss, col, ',')) {
        cols.push_back(col);
    }

    if (cols.empty()) return row;

    row.date_str = cols[0];
    row.timestamp = parse_date(cols[0]);
    
    if (cols.size() >= 6) {
        row.price_a = safe_stod(cols[1]);
        row.price_b = safe_stod(cols[2]);
        row.vix     = safe_stod(cols[3]);
        row.avg_vix = safe_stod(cols[4]);
        row.z_score = safe_stod(cols[5]);
    }

    return row;
}

std::optional<MarketData> CSVDataProvider::get_next_tick() {
    if (current_index < data_buffer.size()) {
        return data_buffer[current_index++];
    }
    return std::nullopt;
}

std::optional<MarketData> CSVDataProvider::get_tick_at(size_t index) {
    if (index < data_buffer.size()) {
        return data_buffer[index];
    }
    return std::nullopt;
}

size_t CSVDataProvider::total_ticks() const {
    return data_buffer.size();
}

void CSVDataProvider::reset() {
    current_index = 0;
}

void CSVDataProvider::load_all() {
    std::ifstream file(file_path_);
    if (!file.is_open()) {
        std::cerr << "Error: Could not reload file " << file_path_ << std::endl;
        return;
    }
    
    data_buffer.clear(); // Clear existing ticks before reload
    std::string line;
    std::getline(file, line); // Skip header

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        data_buffer.push_back(parse_line(line));
    }
}

} // namespace qr_core