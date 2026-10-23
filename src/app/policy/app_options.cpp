#include "enh/app/app_options.hpp"

namespace enh::app {
namespace {

constexpr std::string_view kGlassQualityPrefix = "--glass-quality=";

} // namespace

std::optional<GlassQuality> parseGlassQuality(std::string_view value) noexcept {
    if (value == "0") {
        return GlassQuality::Opaque;
    }
    if (value == "1") {
        return GlassQuality::Lite;
    }
    if (value == "2") {
        return GlassQuality::Full;
    }
    return std::nullopt;
}

AppOptions parseAppOptions(std::span<const std::string_view> args) noexcept {
    AppOptions options;
    for (const std::string_view arg : args) {
        if (arg == "--glass-testbed") {
            options.glassTestbed = true;
        } else if (arg == "--no-mica") {
            options.noMica = true;
        } else if (arg == "--check-modules") {
            options.checkModules = true;
        } else if (arg.starts_with(kGlassQualityPrefix)) {
            // Valeur hors bornes : ignoree (la derniere option valide l'emporte).
            if (const auto q = parseGlassQuality(arg.substr(kGlassQualityPrefix.size()))) {
                options.forcedGlassQuality = q;
            }
        }
    }
    return options;
}

} // namespace enh::app
