#pragma once

// Options de ligne de commande de enhance.exe (ENH-004). C++20 pur, sans Qt, teste en CI.
//   --glass-testbed        motif de test sous la capsule de verre (QA, controle design)
//   --glass-quality=0|1|2  force la qualite du verre (0 opaque, 1 allegee, 2 complete)
//   --no-mica              repli opaque force (comparaison Mica / opaque, risque A1)
//   --check-modules        verifie les chemins des modules charges (SEC-08) puis quitte

#include "enh/app/glass_quality_governor.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace enh::app {

struct AppOptions {
    bool glassTestbed = false;
    std::optional<GlassQuality> forcedGlassQuality;
    bool noMica = false;
    bool checkModules = false;

    friend bool operator==(const AppOptions&, const AppOptions&) = default;
};

/// "0", "1" ou "2" -> qualite ; toute autre valeur -> nullopt (option ignoree).
[[nodiscard]] std::optional<GlassQuality> parseGlassQuality(std::string_view value) noexcept;

/// Analyse les arguments (sans le nom du programme). Arguments inconnus : ignores.
[[nodiscard]] AppOptions parseAppOptions(std::span<const std::string_view> args) noexcept;

} // namespace enh::app
