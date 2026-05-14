#include <iostream>
#include <filesystem>
#include <quant/core/types.hpp>
#include <quant/core/csv_data_provider.hpp>

#include "engines/trading_engine.cpp"


int main(int argc, char* argv[]) {
    // parse the given filepath
    // Check if a filename was provided as a command-line argument. if not, throw an error
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_csv_file>" << std::endl;
        return 1;
    }

    // Extract the path from the arguments
    std::string csvPath(argv[1]);

    // Choose the Data Source
    qr_core::CSVDataProvider csv_source(csvPath);

    // 2. Choose the Engine
    qr_engine::TradingEngine engine;

    // 3. Press "Go"
    engine.run(csv_source);

    return 0;
}