#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <string>
#include <fstream>


struct Trade {
    std::string entryDate;
    std::string exitDate;
    double pnl;
};

struct MarketData {
    std::string date;
    double priceA;
    double priceB;
    double vix;
    double avgVix;
    double zscore;
    double beta;
};

#endif