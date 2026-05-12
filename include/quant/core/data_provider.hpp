#ifndef DATA_PROVIDER_HPP
#define DATA_PROVIDER_HPP

#include <quant/core/types.hpp> // TODO: check this
#include <vector>
#include <optional>

namespace qr_core {

// The Abstract Base Class (The Interface)
class IDataProvider {
public:
    virtual ~IDataProvider() = default;
    
    // Returns the next available row of data, or nullopt if finished
    virtual std::optional<MarketData> get_next_tick() = 0;
    
    // Reset to the beginning (useful for backtesting)
    virtual void reset() = 0;
};

} // namespace qr_core

#endif