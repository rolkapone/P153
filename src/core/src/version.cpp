#include "enh/version.hpp"

#ifndef ENH_VERSION_STRING
#define ENH_VERSION_STRING "0.1.0"
#endif

namespace enh {

std::string_view versionString() noexcept {
    return ENH_VERSION_STRING;
}

} // namespace enh
