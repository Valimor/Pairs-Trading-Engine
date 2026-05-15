#ifndef GARCH_CALIBRATOR_HPP
#define GARCH_CALIBRATOR_HPP

#include <Eigen/Core>
#include <cmath>
#include <limits>
#include <algorithm>
#include <iostream>

namespace qr_math {

struct GarchParameters {
    double omega;
    double alpha;
    double beta;
    double nll;
};

class GarchCalibrator {
public:
    // Computes Negative Log-Likelihood using Eigen vectors for speed
    static double calculate_nll(const Eigen::VectorXd& residuals, double omega, double alpha, double beta) {
        size_t T = residuals.size();
        if (T < 2) return std::numeric_limits<double>::max();

        // Vector to store conditional variances σ²_t
        Eigen::VectorXd variance = Eigen::VectorXd::Zero(T);
        
        // Seed initial variance with the unconditional variance of the residuals
        double mean_sq = residuals.squaredNorm() / T;
        variance(0) = mean_sq;

        double total_nll = 0.0;

        for (size_t t = 1; t < T; ++t) {
            // GARCH(1,1) recursive formula: σ²_t = ω + α * ε²_{t-1} + β * σ²_{t-1}
            variance(t) = omega + alpha * std::pow(residuals(t - 1), 2) + beta * variance(t - 1);
            
            // Boundary enforcement to prevent log(0) or negative variance
            if (variance(t) <= 1e-8) return std::numeric_limits<double>::max();

            // l_t = ln(σ²_t) + (ε²_t / σ²_t)
            total_nll += std::log(variance(t)) + (std::pow(residuals(t), 2) / variance(t));
        }

        return 0.5 * total_nll;
    }

    // Calibrates parameters using a basic gradient descent setup over Eigen types
    GarchParameters fit(const Eigen::VectorXd& spreads) {
        // 1. Calculate residuals (first difference of the spread: ε_t = S_t - S_{t-1})
        size_t T = spreads.size();
        Eigen::VectorXd residuals = Eigen::VectorXd::Zero(T - 1);
        for (size_t t = 1; t < T; ++t) {
            residuals(t - 1) = spreads(t) - spreads(t - 1);
        }

        // 2. Optimization initial configurations
        double omega = 0.000005;
        double alpha = 0.05;
        double beta = 0.85;
        
        double learning_rate = 0.0001;
        size_t max_iterations = 100;
        double h = 1e-5; // Finite difference step size

        double current_nll = calculate_nll(residuals, omega, alpha, beta);

        // 3. Gradient Descent Loop
        for (size_t iter = 0; iter < max_iterations; ++iter) {
            // Approximate partial derivatives using central finite differences
            double d_alpha = (calculate_nll(residuals, omega, alpha + h, beta) - 
                              calculate_nll(residuals, omega, alpha - h, beta)) / (2.0 * h);
                              
            double d_beta  = (calculate_nll(residuals, omega, alpha, beta + h) - 
                              calculate_nll(residuals, omega, alpha, beta - h)) / (2.0 * h);

            // Update candidate parameters
            double next_alpha = alpha - learning_rate * d_alpha;
            double next_beta  = beta - learning_rate * d_beta;

            // Enforce economic and stability constraints: α >= 0, β >= 0, α + β < 0.99
            next_alpha = std::max(0.001, std::min(0.30, next_alpha));
            next_beta  = std::max(0.50, std::min(0.98, next_beta));
            
            if (next_alpha + next_beta >= 0.99) {
                // If constraints are violated, scale them back into the boundary
                double total = next_alpha + next_beta;
                next_alpha = (next_alpha / total) * 0.95;
                next_beta = (next_beta / total) * 0.95;
            }

            double next_nll = calculate_nll(residuals, omega, next_alpha, next_beta);

            // Convergence criteria
            if (std::abs(current_nll - next_nll) < 1e-5) {
                alpha = next_alpha;
                beta = next_beta;
                current_nll = next_nll;
                break;
            }

            alpha = next_alpha;
            beta = next_beta;
            current_nll = next_nll;
        }

        return GarchParameters{omega, alpha, beta, current_nll};
    }
};

} // namespace qr_math
#endif