#include <quant/core/garch_calibrator.hpp>
#include <cmath>
#include <limits>
#include <algorithm>

namespace qr_math {

double GarchCalibrator::calculate_nll(const Eigen::VectorXd& residuals, 
                                      const Eigen::VectorXd& exo_data, 
                                      double omega, double alpha, double beta, double gamma) 
{
    Eigen::Index T = residuals.size();
    if (T < 2 || exo_data.size() != static_cast<size_t>(T)) {
        return std::numeric_limits<double>::max();
    }

    Eigen::VectorXd variance = Eigen::VectorXd::Zero(T);
    
    // Seed initial variance using unconditional variance of residuals
    double mean_sq = residuals.squaredNorm() / T;
    variance(0) = mean_sq;

    double total_nll = 0.0;

    for (size_t t = 1; t < T; ++t) {
        // GARCH-X Recursive Step: σ²_t = ω + α * ε²_{t-1} + β * σ²_{t-1} + γ * X_{t-1}
        variance(t) = omega + 
                      alpha * (residuals(t - 1) * residuals(t - 1)) + 
                      beta * variance(t - 1) + 
                      gamma * exo_data(t - 1);
        
        // Strict lower boundary check to prevent negative variance or log(0)
        if (variance(t) <= 1e-8) return std::numeric_limits<double>::max();

        // Normal Negative Log-Likelihood contribution element
        total_nll += std::log(variance(t)) + ((residuals(t) * residuals(t)) / variance(t));
    }

    return 0.5 * total_nll;
}

GarchParameters GarchCalibrator::fit(const Eigen::VectorXd& spreads, const Eigen::VectorXd& exo_data) {
    // Safety guard rails
    Eigen::Index T = spreads.size();
    if (T < 3 || exo_data.size() != static_cast<size_t>(T)) {
        return GarchParameters{0.000005, 0.05, 0.85, 0.01, std::numeric_limits<double>::max()};
    }

    // 1. Calculate residuals (first difference of the spread)
    Eigen::VectorXd residuals = Eigen::VectorXd::Zero(T - 1);
    Eigen::VectorXd truncated_exo = Eigen::VectorXd::Zero(T - 1);
    
    for (size_t t = 1; t < T; ++t) {
        residuals(t - 1) = spreads(t) - spreads(t - 1);
        truncated_exo(t - 1) = exo_data(t - 1); // Align exogenous timelines
    }

    // 2. Hyperparameter Optimization Starting Estimates
    double omega = 0.000005;
    double alpha = 0.05;
    double beta = 0.85;
    double gamma = 0.01; // Seed value for exogenous influence
    
    double learning_rate = 0.0001;
    size_t max_iterations = 150;
    double h = 1e-5; 

    double current_nll = calculate_nll(residuals, truncated_exo, omega, alpha, beta, gamma);

    // 3. Gradient Descent Optimization Loop (Central Finite Differences)
    for (size_t iter = 0; iter < max_iterations; ++iter) {
        double d_alpha = (calculate_nll(residuals, truncated_exo, omega, alpha + h, beta, gamma) - 
                          calculate_nll(residuals, truncated_exo, omega, alpha - h, beta, gamma)) / (2.0 * h);
                          
        double d_beta  = (calculate_nll(residuals, truncated_exo, omega, alpha, beta + h, gamma) - 
                          calculate_nll(residuals, truncated_exo, omega, alpha, beta - h, gamma)) / (2.0 * h);

        double d_gamma = (calculate_nll(residuals, truncated_exo, omega, alpha, beta, gamma + h) - 
                          calculate_nll(residuals, truncated_exo, omega, alpha, beta, gamma - h)) / (2.0 * h);

        // Gradient Descent Adjustments
        double next_alpha = alpha - learning_rate * d_alpha;
        double next_beta  = beta - learning_rate * d_beta;
        double next_gamma = gamma - learning_rate * d_gamma;

        // Boundary Constraints Enforcement
        next_alpha = std::max(0.001, std::min(0.30, next_alpha));
        next_beta  = std::max(0.50, std::min(0.98, next_beta));
        next_gamma = std::max(0.0001, std::min(0.20, next_gamma));
        
        // Cointegration / Stationarity Wall: α + β < 0.99
        if (next_alpha + next_beta >= 0.99) {
            double total = next_alpha + next_beta;
            next_alpha = (next_alpha / total) * 0.95;
            next_beta = (next_beta / total) * 0.95;
        }

        double next_nll = calculate_nll(residuals, truncated_exo, omega, next_alpha, next_beta, next_gamma);

        // Convergence Check
        if (std::abs(current_nll - next_nll) < 1e-5) {
            alpha = next_alpha;
            beta = next_beta;
            gamma = next_gamma;
            current_nll = next_nll;
            break;
        }

        alpha = next_alpha;
        beta = next_beta;
        gamma = next_gamma;
        current_nll = next_nll;
    }

    return GarchParameters{omega, alpha, beta, gamma, current_nll};
}

} // namespace qr_math