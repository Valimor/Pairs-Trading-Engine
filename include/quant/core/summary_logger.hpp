#ifndef SUMMARY_LOGGER_HPP
#define SUMMARY_LOGGER_HPP

#include <string>
#include <vector>
#include <quant/core/types.hpp>

namespace qr_core {


class SummaryLogger {
private:
    std::string filename_;

public:
    explicit SummaryLogger(const std::string& filename);
    void log_results(const std::vector<OptimizationResult>& results);
};

} // namespace qr_core

#endif // SUMMARY_LOGGER_HPP