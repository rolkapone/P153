#include "enh/demo_counter.hpp"

#include <limits>

namespace enh {

bool DemoCounter::increment() noexcept {
    if (count_ == std::numeric_limits<int>::max()) {
        return false;
    }
    ++count_;
    return true;
}

} // namespace enh
