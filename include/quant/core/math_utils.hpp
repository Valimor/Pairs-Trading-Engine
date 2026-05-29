#ifndef QUANT_MATH_UTILS_HPP
#define QUANT_MATH_UTILS_HPP

#include <vector>
#include <deque>
#include <cmath>
#include <numeric>
#include <algorithm>

namespace qr_math {

    // --- 1. BASIC PAIRS ARITHMETIC ---
    namespace basic {
        inline double calculate_spread(double price_a, double price_b, double beta) {
            return price_a - (beta * price_b);
        }

        inline double calculate_zscore(double spread, double mean, double std_dev) {
            return (std_dev > 1e-9) ? (spread - mean) / std_dev : 0.0;
        }
    }

    // --- 2. STATISTICAL MODELS ---
    namespace stats {
        struct RegressionResult {
            double beta;
            double intercept;
        };

        // Transformed to templates using const-references to eliminate container copy overhead
        template <typename Container>
        inline double calculate_mean(const Container& v) {
            if (v.empty()) return 0.0;
            return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
        }

        template <typename Container>
        inline double calculate_stddev(const Container& v, double mean) {
            if (v.size() < 2) return 0.0;
            double sq_sum = 0;
            for (double x : v) sq_sum += (x - mean) * (x - mean);
            return std::sqrt(sq_sum / (v.size() - 1));
        }

        inline RegressionResult calculate_ols(const std::deque<double>& y, const std::deque<double>& x) {
            if (y.size() != x.size() || y.empty()) return {0.0, 0.0};
            
            size_t n = y.size();
            double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;

            for (size_t i = 0; i < n; ++i) {
                sumX += x[i]; sumY += y[i];
                sumXY += x[i] * y[i]; sumX2 += x[i] * x[i];
            }

            double xMean = sumX / n;
            double yMean = sumY / n;
            double numerator = sumXY - (n * xMean * yMean);
            double denominator = sumX2 - (n * xMean * xMean);

            double beta = (std::abs(denominator) > 1e-9) ? numerator / denominator : 0.0;
            double intercept = yMean - (beta * xMean);
            return {beta, intercept};
        }

        // TODO: check if this should still be a deque
        // I suspect yes
        inline double calculate_half_life(const std::deque<double>& spread_history) {
            if (spread_history.size() < 10) return 0.0;

            std::deque<double> deltaS;
            std::deque<double> laggedS;

            for (size_t i = 1; i < spread_history.size(); ++i) {
                deltaS.push_back(spread_history[i] - spread_history[i-1]);
                laggedS.push_back(spread_history[i-1]);
            }

            auto reg = calculate_ols(deltaS, laggedS);
            if (reg.beta >= 0) return 999.0; // No mean reversion detected

            return -std::log(2) / reg.beta;
        }
    }

    // --- 3. VOLATILITY & SIGNAL SCALING ---
    namespace signals {
        inline double get_vix_scaled_threshold(double base_threshold, double current_vix, double avg_vix) {
            if (avg_vix <= 0) return base_threshold;
            double vol_ratio = current_vix / avg_vix;
            return base_threshold * std::max(0.8, std::sqrt(vol_ratio));
        }

        inline double get_vol_adjusted_threshold(double base_threshold, double current_vix, double avg_vix) {
            if (avg_vix == 0) return base_threshold;
            double vol_ratio = current_vix / avg_vix;
            return base_threshold * std::sqrt(vol_ratio);
        } 

        inline double get_garch_threshold(double base_threshold, double last_residual, double last_var, 
                                          double omega, double alpha, double beta) {
            double predicted_var = omega + (alpha * std::pow(last_residual, 2)) + (beta * last_var);
            double predicted_sigma = std::sqrt(predicted_var);

            const double baseline_sigma = 0.03; 
            return base_threshold * (predicted_sigma / baseline_sigma);
        }
    }

    // --- 4. PERFORMANCE METRICS ---
    namespace performance {
        inline double calculate_sharpe(const std::vector<double>& trade_pnls, double initial_capital) {
            if (trade_pnls.size() < 2) return 0.0;

            std::vector<double> returns;
            returns.reserve(trade_pnls.size()); // Pre-allocate memory optimization
            for (double pnl : trade_pnls) {
                returns.push_back(pnl / initial_capital);
            }

            double mean = stats::calculate_mean(returns);
            double std_dev = stats::calculate_stddev(returns, mean);

            if (std_dev < 1e-9) return 0.0;

            return (mean / std_dev) * std::sqrt(252);
        }
    }
}

#endif // QUANT_MATH_UTILS_HPP