#pragma once

// SEC-13 : toute taille en std::size_t, multiplications et additions verifiees.

#include "enh/pipeline/error.hpp"

#include <cstddef>
#include <limits>
#include <string>

namespace enh::pipeline {

/// a * b, ou UnsupportedFormat si le resultat deborde de std::size_t.
[[nodiscard]] inline Result<std::size_t> checkedMul(std::size_t a, std::size_t b) {
    if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "size overflow: " + std::to_string(a) + " * " + std::to_string(b));
    }
    return a * b;
}

/// a + b, ou UnsupportedFormat si le resultat deborde de std::size_t.
[[nodiscard]] inline Result<std::size_t> checkedAdd(std::size_t a, std::size_t b) {
    if (b > std::numeric_limits<std::size_t>::max() - a) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "size overflow: " + std::to_string(a) + " + " + std::to_string(b));
    }
    return a + b;
}

/// Arrondit `value` au multiple superieur de `alignment` (puissance de 2), avec verification.
[[nodiscard]] inline Result<std::size_t> alignUp(std::size_t value, std::size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return fail(PipelineErrorCode::InvalidParams,
                    "alignment must be a power of two: " + std::to_string(alignment));
    }
    auto sum = checkedAdd(value, alignment - 1);
    if (!sum) {
        return sum;
    }
    return *sum & ~(alignment - 1);
}

} // namespace enh::pipeline
