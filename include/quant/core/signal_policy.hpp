#ifndef SIGNAL_POLICY_HPP
#define SIGNAL_POLICY_HPP

#include <quant/core/types.hpp>
#include <quant/core/math_utils.hpp>

namespace qr_engine {

class ISignalPolicy {
public:
    virtual ~ISignalPolicy() = default;
    virtual double get_threshold(const qr_core::MarketData& tick) = 0;
};

class VixScaledPolicy : public ISignalPolicy {
public:
    double get_threshold(const qr_core::MarketData& tick) override {
        return qr_math::signals::get_vix_scaled_threshold(2.0, tick.vix, tick.avg_vix);
    }
};

class RollingVolPolicy : public ISignalPolicy {
private:
    double base_threshold_;
    
public:
    RollingVolPolicy(double base = 2.0) : base_threshold_(base) {}

    double get_threshold(const qr_core::MarketData& tick) override {
        // We use the VIX and AvgVix as proxies for rolling volatility
        if (tick.avg_vix <= 0) return base_threshold_;

        double vol_ratio = tick.vix / tick.avg_vix;
        
        // We only scale UP (std::max(1.0, ...)) to ensure we don't 
        // make the strategy too aggressive during low-volatility regimes.
        return base_threshold_ * std::max(1.0, vol_ratio);
    }
};

class GarchPolicy : public ISignalPolicy {
private:
    double base_threshold_;
    double last_variance_ = 0.0009; // Initial seed (approx 3% daily vol squared)
    double last_residual_ = 0.0;
    
    // Standard GARCH(1,1) parameters for equities
    const double omega = 0.000005; 
    const double alpha = 0.04;     
    const double beta = 0.90;      
    const double baseline_sigma = 0.03;

    //TODO: add functions to perform maximum likelihood estimation on these parameters
    // to update them in real time to better fit market behaviors

public:
    GarchPolicy(double base = 2.0) : base_threshold_(base) {}

    double get_threshold(const qr_core::MarketData& tick) override {
        // 1. Predict today's variance: σ²_t = ω + αε²_{t-1} + βσ²_{t-1}
        double predicted_var = omega + (alpha * std::pow(last_residual_, 2)) + (beta * last_variance_);
        double predicted_sigma = std::sqrt(predicted_var);

        // 2. Update state for the NEXT tick
        // In a real system, residual is (Actual_Spread - Predicted_Spread)
        // Here we use the Z-score as a proxy for the 'shock' to the system
        last_residual_ = tick.z_score * predicted_sigma; 
        last_variance_ = predicted_var;

        // 3. Scale the threshold
        return base_threshold_ * (predicted_sigma / baseline_sigma);
    }
};

}

#endif