#include <quant/core/signal_policy.hpp>
#include <quant/core/math_utils.hpp>
#include <cmath>
#include <algorithm>

namespace qr_engine {

// ============================================================================
// VixScaledPolicy Implementation
// ============================================================================
double VixScaledPolicy::get_threshold(const qr_core::MarketData& tick) {
    return qr_math::signals::get_vix_scaled_threshold(2.0, tick.vix, tick.avg_vix);
}

// ============================================================================
// RollingVolPolicy Implementation
// ============================================================================
RollingVolPolicy::RollingVolPolicy(double base) : base_threshold_(base) {}

double RollingVolPolicy::get_threshold(const qr_core::MarketData& tick) {
    if (tick.avg_vix <= 0) return base_threshold_;

    double vol_ratio = tick.vix / tick.avg_vix;
    
    // Scale UP only to prevent over-aggression in low-volatility environments
    return base_threshold_ * std::max(1.0, vol_ratio);
}

// ============================================================================
// GarchPolicy Implementation
// ============================================================================
GarchPolicy::GarchPolicy(double entry_z, double base, double a, double b, double w) 
    : entry_z_(entry_z),
      base_threshold_(base), 
      alpha_(a), 
      beta_(b), 
      omega_(w) {}

double GarchPolicy::get_threshold(const qr_core::MarketData& tick) {
   // Correct GARCH-X equation updating variance using live exogenous VIX inputs
    double predicted_var = omega_ + 
                          (alpha_ * std::pow(last_residual_, 2)) + 
                          (beta_ * last_variance_) + 
                          (gamma_ * tick.vix); // <-- FIXED: Ingest external feature
                          
    double predicted_sigma = std::sqrt(predicted_var);

    last_residual_ = tick.z_score * predicted_sigma; 
    last_variance_ = predicted_var;

    return base_threshold_ * (predicted_sigma / baseline_sigma);
}

} // namespace qr_engine