#pragma once

// Alias unique de enh::Expected (ADR-003). Necessite tl-expected (vcpkg) : en phase A, ne pas
// inclure ce fichier hors d'un bloc #ifdef ENH_HAVE_EXPECTED. Migration vers std::expected
// (C++23) = changer ce seul fichier.
#ifndef ENH_HAVE_EXPECTED
#error "enh/expected.hpp demande tl-expected : installer les dependances (scripts/bootstrap.ps1)."
#endif

#include <tl/expected.hpp>

namespace enh {

template <typename T, typename E> using Expected = tl::expected<T, E>;

template <typename E> using Unexpected = tl::unexpected<E>;

} // namespace enh
