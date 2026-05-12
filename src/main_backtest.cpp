#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <quant/core/strategy.hpp>
#include <filesystem> // C++17
namespace fs = std::filesystem;

// This function converts the CSV text file into a Vector of MarketData objects
std::vector<MarketData> load_data(const std::string& filename) {
    std::vector<MarketData> data;
    std::ifstream file(filename);
    std::string line, word;

    if (!file.is_open()) {
        std::cerr << "Could not open the file!" << std::endl;
        return data;
    }

    // skip header row
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        MarketData row;
        std::string col;
        std::vector<std::string> columns;

        while (std::getline(ss, col, ',')) {
            columns.push_back(col);
        }

        // CSV mapping: 0:Date, 1:PriceA, 2:PriceB, 3:ZScore
        if (columns.size() >= 4) {
            row.date = columns[0];
            row.priceA = std::stod(columns[1]);
            row.priceB = std::stod(columns[2]);
            row.vix = std::stod(columns[3]);
            row.avgVix = std::stod(columns[4]);
            row.zscore = std::stod(columns[5]);
            //row.beta = std::stod(columns[6]); TODO: IMPLEMENT THIS!
            data.push_back(row);
        }
    }
    return data;
}

int main(int argc, char* argv[]) {
    
    // Check if a filename was provided as a command-line argument. if not, throw an error
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_csv_file>" << std::endl;
        return 1;
    }

    // Extract the path from the arguments
    std::string csvPath(argv[1]);

    auto market_records = load_data(csvPath);

    if (market_records.empty()) return 1;

    Backtester engine;
    double threshold;
  
    for (const auto& record : market_records) {
        //compute the threshold
        if(engine.getCurrentStrat() == Strategy::VIX) {
            threshold = qr_math::getVixScaledThreshold(engine.getDefaultThreshold(), record.vix, record.avgVix);
        }
        else if(engine.getCurrentStrat() == Strategy::ROLL_VOL) {
            threshold = qr_math::calcRollingVolThreshold(
                engine.getDefaultThreshold(), 
                engine.getCurrentSpreadStd(), 
                engine.getHistoricalAvgStd()
            );
        }
        else if(engine.getCurrentStrat() == Strategy::GARCH) {
            threshold = qr_math::calcGarchThreshold(
                engine.getDefaultThreshold(), 
                engine.getLastResidual(), 
                engine.getLastVariance()
            );
        }
        else {
            threshold = engine.getDefaultThreshold();
        }
        engine.execute(record.priceA, record.priceB, threshold, record.date);
    }
    std::cout << "Final PnL: $" << engine.getTotalPnL() << std::endl;
    return 0;
}