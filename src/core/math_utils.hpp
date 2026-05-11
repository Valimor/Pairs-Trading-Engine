#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

#include <vector>
#include <deque>
#include <cmath>
#include <numeric>

namespace qr_math {

    struct RegressionResult {
        double beta;
        double intercept;
    };

    // Calculate OLS Beta and Intercept
    inline RegressionResult calculateRollingBeta(const std::deque<double>& y, const std::deque<double>& x) {
        if (y.size() != x.size() || y.empty()) return {0.0, 0.0};
        
        size_t n = y.size();
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;

        for (size_t i = 0; i < n; ++i) {
            sumX += x[i];
            sumY += y[i];
            sumXY += x[i] * y[i];
            sumX2 += x[i] * x[i];
        }

        double xMean = sumX / n;
        double yMean = sumY / n;

        double numerator = sumXY - (n * xMean * yMean);
        double denominator = sumX2 - (n * xMean * xMean);

        double beta = (std::abs(denominator) > 1e-9) ? numerator / denominator : 0.0;
        double intercept = yMean - (beta * xMean);

        return {beta, intercept};
    }

    inline double calculateMean(const std::deque<double>& v) {
        if (v.empty()) return 0.0;
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    }

    inline double calculateStdDev(const std::deque<double>& v, double mean) {
        if (v.size() < 2) return 0.0;
        double sq_sum = 0;
        for (double x : v) sq_sum += (x - mean) * (x - mean);
        return std::sqrt(sq_sum / (v.size() - 1));
    }

    inline double calculateSharpe(const std::vector<double>& returns) {
        if (returns.size() < 2) return 0.0;
        double mean = calculateMean(std::deque<double>(returns.begin(), returns.end()));
        double stdDev = calculateStdDev(std::deque<double>(returns.begin(), returns.end()), mean);
        
        if (stdDev == 0) return 0.0;
        // Annualization factor (assuming daily data, ~252 trading days)
        return (mean / stdDev) * std::sqrt(252);
    }

    inline double calculateHalfLife(const std::deque<double>& spreadHistory) {
        if (spreadHistory.size() < 10) return 0.0;

        std::deque<double> deltaS;
        std::deque<double> laggedS;

        // Create the ΔS and S_{t-1} vectors
        for (size_t i = 1; i < spreadHistory.size(); ++i) {
            deltaS.push_back(spreadHistory[i] - spreadHistory[i-1]);
            laggedS.push_back(spreadHistory[i-1]);
        }

        // Reuse your existing regression logic: ΔS = beta * S_{t-1} + alpha
        auto reg = calculateRollingBeta(deltaS, laggedS);
        
        // lambda is -beta. We need beta to be negative for mean reversion to exist.
        if (reg.beta >= 0) return 999.0; // Indicates no mean reversion detected

        return -std::log(2) / reg.beta;
    }

    // VOLATILITY ADJUSTMENTS
    // 1. SIMPLEST: VIX-BASED SCALING
    // Logic: Scales a base threshold by the ratio of current fear vs historical fear.
    inline double getVixScaledThreshold(double baseThreshold, double currentVix, double avgVix) {
        if (avgVix <= 0) return baseThreshold;
        double multiplier = currentVix / avgVix;
        // We use std::max to ensure we don't drop below a "sane" minimum threshold
        return baseThreshold * std::max(0.8, multiplier);
    }

    // 2. MEDIUM: ROLLING VOLATILITY NORMALIZATION
    // Logic: Increases threshold if the spread's current volatility is higher than its recent average.
    inline double calcRollingVolThreshold(double baseThreshold, double currentVol, double avgVol) {
        // If we don't have enough data yet, return the base
        if (avgVol <= 0) return baseThreshold;

        // Compute the ratio of current noise vs historical noise
        double volRatio = currentVol / avgVol;

        // We use std::max(1.0, ...) to ensure we only SCALE UP during high volatility.
        // We don't want the threshold to get easier (lower than 2.0) during low volatility.
        return baseThreshold * std::max(1.0, volRatio);
    }

    // 3. ADVANCED: GARCH(1,1) INSPIRED VOLATILITY FORECAST
    // Logic: Predicts 'tomorrow's' variance based on past residuals and past variance.
    // This returns a predicted Sigma, which you use to scale your threshold.
    inline double calcGarchThreshold(double baseThreshold, double lastResidual, double lastVar) {
        // Typical GARCH(1,1) parameters for daily equity data
        const double omega = 0.000005; // Long-run variance constant
        const double alpha = 0.04;     // Sensitivity to recent shocks (residuals)
        const double beta = 0.90;      // Persistence of volatility

        // Formula: σ²_t = ω + α * ε²_{t-1} + β * σ²_{t-1}
        double predictedVar = omega + (alpha * std::pow(lastResidual, 2)) + (beta * lastVar);
        double predictedSigma = std::sqrt(predictedVar);

        // We scale the threshold based on how much the predicted sigma 
        // deviates from a "baseline" sigma (e.g., 0.02 or 2% daily vol)
        double baselineSigma = 0.03; 
        return baseThreshold * (predictedSigma / baselineSigma);
    }
}

#endif