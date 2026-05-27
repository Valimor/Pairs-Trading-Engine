#ifndef GARCH_CALIBRATOR_HPP
#define GARCH_CALIBRATOR_HPP

#include <Eigen/Core>

namespace qr_math {

struct GarchParameters {
    double omega;
    double alpha;
    double beta;
    double gamma;
    double nll;
};

class GarchCalibrator {
private:
    // Kept private since users of the calibrator only care about the final fit results
    static double calculate_nll(const Eigen::VectorXd& residuals, const Eigen::VectorXd& exo_data, 
        double omega, double alpha, double beta, double gamma);

public:
    GarchCalibrator() = default;
    
    // Clear blueprint interface
    GarchParameters fit(const Eigen::VectorXd& spreads, const Eigen::VectorXd& exo_data);
};

} // namespace qr_math
#endif // GARCH_CALIBRATOR_HPP