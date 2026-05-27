#ifndef SIGNAL_POLICY_HPP
#define SIGNAL_POLICY_HPP

#include <quant/core/types.hpp>

namespace qr_engine {

// --- ABSTRACT BASE INTERFACE ---
class ISignalPolicy {
public:
    virtual ~ISignalPolicy() = default;
    virtual double get_threshold(const qr_core::MarketData& tick) = 0;
};

// --- VIX SCALED POLICY ---
class VixScaledPolicy : public ISignalPolicy {
public:
    double get_threshold(const qr_core::MarketData& tick) override;
};

// --- ROLLING VOLATILITY POLICY ---
class RollingVolPolicy : public ISignalPolicy {
private:
    double base_threshold_;
    
public:
    explicit RollingVolPolicy(double base = 2.0);
    double get_threshold(const qr_core::MarketData& tick) override;
};

// --- GARCH(1,1) DYNAMIC POLICY ---
class GarchPolicy : public ISignalPolicy {
private:
    double entry_z_;
    double base_threshold_;
    double alpha_;
    double beta_;
    double omega_;
    double gamma_;  
    double last_variance_; 
    double last_residual_; 
    const double baseline_sigma = 0.03;

public:
    GarchPolicy(double entry_z, double base, double a, double b, double w = 0.000005);
    double get_threshold(const qr_core::MarketData& tick) override;
};

} // namespace qr_engine

#endif // SIGNAL_POLICY_HPP