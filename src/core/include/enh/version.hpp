#pragma once

#include <string_view>

namespace enh {

/// Version du produit, "major.minor.patch" (source unique : CMake project(VERSION)).
[[nodiscard]] std::string_view versionString() noexcept;

} // namespace enh
