#ifndef STRATEGY_HPP
#define STRATEGY_HPP

#include <vector>
#include <string>
#include <fstream>

#include "math_utils.hpp"

enum class Position { FLAT, LONG, SHORT };

enum class Strategy { CONST, VIX, ROLL_VOL, GARCH };

struct Trade {
    std::string entryDate;
    std::string exitDate;
    double pnl;
};

// Inside strategy.hpp
struct MarketData {
    std::string date;
    double priceA;
    double priceB;
    double vix;
    double avgVix;
    double zscore;
    double beta;
};

class Backtester {
private:
    // State Management
    Position currentPos = Position::FLAT;
    std::deque<double> historyA;
    std::deque<double> historyB;
    std::deque<double> spreadHistory;

    // Config
    const Strategy currentStrat = Strategy::ROLL_VOL;
    size_t windowSize = 100;
    double capital = 100000.0; 
    double allocationPerLeg = 50000.0;
    double defaultThreshold = 2.0;

    //logFile variables
    double previousEquity = capital;
    std::vector<double> dailyReturns;
    std::ofstream logFile;
    std::ofstream tradeFile;

    //volatility
    std::deque<double> spreadVolHistory;
    const size_t volLookback = 30; // Rolling window for "average" volatility

    double lastResidual = 0.0;
    double lastVariance = 0.0004; // Baseline seed
    double currentSpreadStd = 0.0;
    
    // Tracking specific trade details
    // Active Trade Data
    double sharesA = 0.0;
    double sharesB = 0.0;
    double entryPriceA = 0.0;
    double entryPriceB = 0.0;
    double lockedHedgeRatio = 0.0;
    double totalPnL = 0.0;
    std::string entryDate;

public:
    Backtester() {
        logFile.open("data/logs/backtest_log.csv");
        logFile << "Date,PriceA,PriceB,ZScore,Sharpe,Beta,PnL,Equity,Window Size\n";
        tradeFile.open("data/logs/backtest_trades_log.csv");
        tradeFile << "Entry Date,Exit Date,P&L A,P&L B,P&L\n";
    }

    ~Backtester() {
        if (logFile.is_open()) logFile.close();
        if (tradeFile.is_open()) tradeFile.close();
    }

    // This is the core update logic
    void updateVolatilityMetrics(double currentSpread, double mean, double stdDev) {
        // 1. Update current standard deviation
        currentSpreadStd = stdDev;

        // 2. Update Rolling Vol History
        spreadVolHistory.push_back(stdDev);
        if (spreadVolHistory.size() > volLookback) {
            spreadVolHistory.pop_front();
        }

        // 3. Update GARCH Residual (Spread - Rolling Mean)
        lastResidual = currentSpread - mean;
        
        // 4. Update GARCH Variance (Iterative update)
        // This keeps the variance current for the NEXT threshold calculation
        //TODO: make it so these hardcoded numbers are tunable
        lastVariance = 0.000005 + (0.05 * std::pow(lastResidual, 2)) + (0.90 * lastVariance);
    }

    void execute(double pA, double pB, double threshold, std::string date) {     
        historyA.push_back(pA);
        historyB.push_back(pB);

        if (historyA.size() < windowSize) return;
        if (historyA.size() > windowSize) {
            historyA.pop_front();
            historyB.pop_front();
        }

        // 1. Calculate the CURRENT Beta/Intercept
        auto reg = qr_math::calculateRollingBeta(historyA, historyB);
        
        // 2. Calculate the current spread point
        double currentSpread = pA - (reg.beta * pB + reg.intercept);
        spreadHistory.push_back(currentSpread);

        //compute the window size based on the half-life
        if (spreadHistory.size() >= 60) { // assumes too little variance to be captured if leq 60
            windowSize = 2 * qr_math::calculateHalfLife(spreadHistory);
        }

        if (spreadHistory.size() < windowSize) return;

        // window size is not being taken into account properly here.
        if (spreadHistory.size() > windowSize) spreadHistory.pop_front();

        // 3. Calculate Z-Score
        double mean = qr_math::calculateMean(spreadHistory);
        double stdDev = qr_math::calculateStdDev(spreadHistory, mean);
        
        // Avoid division by zero if prices are flat
        double zscore = (stdDev > 1e-6) ? (currentSpread - mean) / stdDev : 0.0;

        // update volatility
        updateVolatilityMetrics(currentSpread, mean, stdDev);

        // compute sharpe ratio and determine if the trade is high quality in that moment
        double historicalSharpe = qr_math::calculateSharpe(dailyReturns);
        bool qualityFilter = (historicalSharpe > 1.0 || dailyReturns.size() < 30);

        //log the data in the logfile
        logFile << date << "," << pA << "," << pB << "," << zscore << "," << historicalSharpe << ","
                << reg.beta << "," << totalPnL << "," << getFinalBalance() << "," << windowSize << "\n";

        // 4. Execution Logic
        if (currentPos == Position::FLAT && qualityFilter) {
            if (zscore > threshold || zscore < -1*threshold) {
                // Lock in the Beta at the moment of execution
                lockedHedgeRatio = reg.beta;
                entryPriceA = pA;
                entryPriceB = pB;

                sharesA = allocationPerLeg / pA;
                sharesB = sharesA * lockedHedgeRatio;

                currentPos = (zscore > 2.0) ? Position::SHORT : Position::LONG;

                //logic to track the trades:
                entryDate = date;
            }
        } 
        else {
            // EXIT LOGIC
            bool exitSignal = (currentPos == Position::SHORT && zscore <= 0.0) || 
                              (currentPos == Position::LONG && zscore >= 0.0);

            if (exitSignal) {
                double pnlA = (currentPos == Position::LONG) ? sharesA * (pA - entryPriceA) : sharesA * (entryPriceA - pA);
                double pnlB = (currentPos == Position::LONG) ? sharesB * (entryPriceB - pB) : sharesB * (pB - entryPriceB);

                totalPnL += (pnlA + pnlB);
                currentPos = Position::FLAT;

                tradeFile << entryDate << "," << date << "," << pnlA << "," << pnlB << "," << pnlA + pnlB << "\n";
            }
        }

        //update the equity
        double unrealizedPnL = 0.0;
        if (currentPos == Position::LONG) {
            unrealizedPnL = sharesA * (pA - entryPriceA) + sharesB * (entryPriceB - pB);
        } else if (currentPos == Position::SHORT) {
            unrealizedPnL = sharesA * (entryPriceA - pA) + sharesB * (pB - entryPriceB);
        }

        double currentEquity = capital + totalPnL + unrealizedPnL;

        // 2. Calculate Daily Return
        double dailyRet = (currentEquity - previousEquity) / previousEquity;
        dailyReturns.push_back(dailyRet);

        // 3. Update previousEquity for the next day
        previousEquity = currentEquity;
    }

    double getHistoricalAvgStd() const {
        if (spreadVolHistory.empty()) return 0.0;
        double sum = std::accumulate(spreadVolHistory.begin(), spreadVolHistory.end(), 0.0);
        return sum / spreadVolHistory.size();
    }

    double getCurrentSpreadStd() const { return currentSpreadStd; }
    double getLastResidual() const { return lastResidual; }
    double getLastVariance() const { return lastVariance; }
    double getFinalBalance() { return capital + totalPnL; }
    double getTotalPnL() { return totalPnL; }
    double getDefaultThreshold() { return defaultThreshold;}
    Strategy getCurrentStrat() {return currentStrat; }
    std::deque<double> getSpreadHistory() {return spreadHistory;}
    std::deque<double> getSpreadVolHistory() {return spreadVolHistory;}
};

#endif