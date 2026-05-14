#include <iostream>
#include <filesystem>
#include <quant/core/types.hpp>
#include <quant/core/csv_data_provider.hpp>

#include "engines/trading_engine.cpp"


int main() {
    // 1. Choose the factory (Data Source)
    qr_core::CSVDataProvider csv_source("C:\\Users\\maxim\\Documents\\GitHub\\QR Projects\\Pairs Trading Engine\\data\\historical\\AMAT_ADI_market_data.csv");

    // 2. Choose the machine (Engine)
    qr_engine::TradingEngine engine;

    // 3. Press "Go"
    engine.run(csv_source);

    return 0;
}