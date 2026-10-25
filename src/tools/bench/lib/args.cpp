#include "enh/bench/args.hpp"

#include <array>
#include <cmath>
#include <set>
#include <string>

namespace enh::bench {

namespace {

using Error = enh::Unexpected<std::string>;

/// UTF-16 -> UTF-8 (messages d'erreur).
std::string narrow(std::wstring_view w) {
    const std::u8string u8 = std::filesystem::path(w).u8string();
    return {u8.begin(), u8.end()};
}

/// Entier decimal sans signe, sans espace ; nullopt si invalide ou hors de [lo, hi].
std::optional<std::uint64_t> parseUnsigned(std::wstring_view text, std::uint64_t lo,
                                           std::uint64_t hi) {
    if (text.empty() || text.size() > 19) {
        return std::nullopt;
    }
    std::uint64_t v = 0;
    for (const wchar_t c : text) {
        if (c < L'0' || c > L'9') {
            return std::nullopt;
        }
        v = v * 10 + static_cast<std::uint64_t>(c - L'0'); // 19 chiffres : pas de debordement
    }
    if (v < lo || v > hi) {
        return std::nullopt;
    }
    return v;
}

/// "LxH" (x ou X), chaque cote dans [1, kMaxOutputSidePx].
std::optional<pipeline::Size> parseSize(std::wstring_view text) {
    const auto x = text.find_first_of(L"xX");
    if (x == std::wstring_view::npos) {
        return std::nullopt;
    }
    const auto w = parseUnsigned(text.substr(0, x), 1, kMaxOutputSidePx);
    const auto h = parseUnsigned(text.substr(x + 1), 1, kMaxOutputSidePx);
    if (!w || !h) {
        return std::nullopt;
    }
    return pipeline::Size{.width = static_cast<std::uint32_t>(*w),
                          .height = static_cast<std::uint32_t>(*h)};
}

struct OptionSpec {
    std::wstring_view name;
    bool takes_value{false};
};

constexpr OptionSpec withValue(std::wstring_view name) noexcept {
    return {.name = name, .takes_value = true};
}
constexpr OptionSpec flag(std::wstring_view name) noexcept {
    return {.name = name, .takes_value = false};
}

constexpr std::array kOptions = {
    withValue(L"--stage"),      withValue(L"--in"),      withValue(L"--out"),
    withValue(L"--report"),     withValue(L"--repeat"),  withValue(L"--warmup"),
    withValue(L"--max-frames"), withValue(L"--decoder"), withValue(L"--q"),
    withValue(L"--bits"),       withValue(L"--size"),    withValue(L"--iterations"),
    withValue(L"--sample-ms"),  flag(L"--force"),        flag(L"--help"),
    flag(L"--allow-corrupt"),
};

const OptionSpec* findOption(std::wstring_view name) {
    for (const OptionSpec& o : kOptions) {
        if (o.name == name) {
            return &o;
        }
    }
    return nullptr;
}

Error invalidValue(std::wstring_view option, std::wstring_view value, std::string_view expected) {
    return Error("valeur invalide pour " + narrow(option) + " : \"" + narrow(value) +
                 "\" (attendu : " + std::string(expected) + ")");
}

} // namespace

enh::Expected<BenchArgs, std::string> parseArgs(std::span<const std::wstring> args) {
    BenchArgs out;
    std::set<std::wstring, std::less<>> seen;
    for (std::size_t i = 0; i < args.size(); ++i) {
        std::wstring_view token = args[i];
        std::wstring_view value;
        bool inline_value = false;
        if (const auto eq = token.find(L'='); token.starts_with(L"--") && eq != token.npos) {
            value = token.substr(eq + 1);
            token = token.substr(0, eq);
            inline_value = true;
        }
        if (token == L"-h" || token == L"/?") {
            token = L"--help";
        }
        const OptionSpec* spec = findOption(token);
        if (spec == nullptr) {
            return Error("option inconnue : " + narrow(args[i]) + " (voir --help)");
        }
        if (!seen.emplace(token).second) {
            return Error("option repetee : " + narrow(token));
        }
        if (spec->takes_value && !inline_value) {
            if (i + 1 >= args.size()) {
                return Error("valeur manquante apres " + narrow(token));
            }
            value = args[++i];
        } else if (!spec->takes_value && inline_value) {
            return Error("l'option " + narrow(token) + " ne prend pas de valeur");
        }

        if (token == L"--help") {
            out.help = true;
        } else if (token == L"--force") {
            out.force = true;
        } else if (token == L"--allow-corrupt") {
            out.allow_corrupt = true;
        } else if (token == L"--stage") {
            if (value == L"decode") {
                out.stage = Stage::Decode;
            } else if (value == L"vsr-image") {
                out.stage = Stage::VsrImage;
            } else {
                return invalidValue(token, value, "decode ou vsr-image");
            }
        } else if (token == L"--in" || token == L"--out" || token == L"--report") {
            if (value.empty()) {
                return invalidValue(token, value, "un chemin de fichier");
            }
            const std::filesystem::path p(value);
            if (token == L"--in") {
                out.input = p;
            } else if (token == L"--out") {
                out.output = p;
            } else {
                out.report = p;
            }
        } else if (token == L"--repeat") {
            const auto v = parseUnsigned(value, 1, kMaxRepeat);
            if (!v) {
                return invalidValue(token, value, "entier de 1 a 1000");
            }
            out.repeat = static_cast<std::uint32_t>(*v);
        } else if (token == L"--warmup") {
            const auto v = parseUnsigned(value, 0, kMaxWarmup);
            if (!v) {
                return invalidValue(token, value, "entier de 0 a 100000");
            }
            out.warmup = static_cast<std::uint32_t>(*v);
        } else if (token == L"--max-frames") {
            const auto v = parseUnsigned(value, 1, 1'000'000'000);
            if (!v) {
                return invalidValue(token, value, "entier de 1 a 1000000000");
            }
            out.max_frames = *v;
        } else if (token == L"--decoder") {
            if (value == L"auto") {
                out.decoder = DecoderMode::Auto;
            } else if (value == L"nvdec") {
                out.decoder = DecoderMode::Nvdec;
            } else if (value == L"software") {
                out.decoder = DecoderMode::Software;
            } else {
                return invalidValue(token, value, "auto, nvdec ou software");
            }
        } else if (token == L"--q") {
            const auto v = parseUnsigned(value, 0, 4);
            if (!v) {
                return invalidValue(token, value, "entier de 0 a 4");
            }
            out.quality = static_cast<std::uint8_t>(*v);
        } else if (token == L"--bits") {
            if (value != L"8" && value != L"10") {
                return invalidValue(token, value, "8 ou 10");
            }
            out.bits = value == L"8" ? std::uint8_t{8} : std::uint8_t{10};
        } else if (token == L"--size") {
            const auto s = parseSize(value);
            if (!s) {
                return invalidValue(token, value, "LARGEURxHAUTEUR, chaque cote de 1 a 8192");
            }
            out.size = *s;
        } else if (token == L"--iterations") {
            const auto v = parseUnsigned(value, 1, kMaxIterations);
            if (!v) {
                return invalidValue(token, value, "entier de 1 a 100000");
            }
            out.iterations = static_cast<std::uint32_t>(*v);
        } else if (token == L"--sample-ms") {
            const auto v = parseUnsigned(value, 1, kMaxSampleMs);
            if (!v) {
                return invalidValue(token, value, "entier de 1 a 1000");
            }
            out.sample_ms = static_cast<std::uint32_t>(*v);
        }
    }
    if (out.help) {
        return out;
    }

    // Combinaisons
    if (!seen.contains(L"--stage")) {
        return Error("--stage est obligatoire (decode ou vsr-image)");
    }
    if (!seen.contains(L"--in")) {
        return Error("--in est obligatoire");
    }
    const auto forbid = [&](std::initializer_list<std::wstring_view> names,
                            std::string_view stage) -> std::optional<std::string> {
        for (const std::wstring_view n : names) {
            if (seen.contains(n)) {
                return narrow(n) + " ne s'applique pas a --stage " + std::string(stage);
            }
        }
        return std::nullopt;
    };
    if (out.stage == Stage::Decode) {
        if (auto e = forbid({L"--out", L"--q", L"--bits", L"--size", L"--iterations"}, "decode")) {
            return Error(*e);
        }
    } else {
        if (auto e = forbid({L"--decoder", L"--max-frames", L"--allow-corrupt"}, "vsr-image")) {
            return Error(*e);
        }
        if (!out.output) {
            return Error("--out est obligatoire avec --stage vsr-image");
        }
    }
    if (out.report && out.output && *out.report == *out.output) {
        return Error("--report et --out designent le meme fichier");
    }
    return out;
}

std::string_view usageText() noexcept {
    return R"(enhance-bench : banc de mesure d'Enhance (outil de developpement, non livre).

  enhance-bench --stage decode    --in <video> [--report <json>] [--repeat N] [--warmup K]
                [--max-frames M] [--decoder auto|nvdec|software] [--sample-ms T] [--force]
                [--allow-corrupt]
  enhance-bench --stage vsr-image --in <image.bmp> --out <image.bmp> [--q 0..4] [--bits 8|10]
                [--size 3840x2160] [--iterations N] [--warmup K] [--repeat N]
                [--report <json>] [--sample-ms T] [--force]

Valeurs par defaut : --repeat 1, --warmup 30, --decoder auto, --q 4, --bits 8,
--size 3840x2160, --iterations 300, --sample-ms 10. Sans --report, le JSON est ecrit sur la
sortie standard. --report et --out refusent un fichier existant, sauf avec --force, et ne
peuvent jamais designer le fichier d'entree. --max-frames M : arret des que M images sont
decodees (au moins M : les images deja sorties du dernier paquet sont comptees).
Decodage partiel (paquet corrompu ou rejete, moins d'images que l'annonce du conteneur) :
code 2 (SourceCorrupt) ; avec --allow-corrupt, code 0 et avertissement (JSON et stderr).

Codes de sortie : 0 succes ; 1 arguments invalides (sans JSON) ou sortie deja existante (JSON
avec error si --report a pu etre ouvert) ; 2 entree illisible ou format non pris en
charge ; 3 GPU, pilote ou composant absent ; 4 VRAM insuffisante ; 5 erreur interne ou ecriture.
)";
}

std::string_view toString(Stage s) noexcept {
    return s == Stage::Decode ? "decode" : "vsr-image";
}

std::string_view toString(DecoderMode d) noexcept {
    switch (d) {
    case DecoderMode::Auto:
        return "auto";
    case DecoderMode::Nvdec:
        return "nvdec";
    case DecoderMode::Software:
        return "software";
    }
    return "auto";
}

pipeline::Size fitInside(pipeline::Size in, pipeline::Size frame) noexcept {
    if (in.width == 0 || in.height == 0) {
        return frame;
    }
    const double sx = static_cast<double>(frame.width) / in.width;
    const double sy = static_cast<double>(frame.height) / in.height;
    const double s = sx < sy ? sx : sy;
    const auto side = [s](std::uint32_t v, std::uint32_t limit) {
        const auto r = static_cast<std::uint32_t>(std::lround(static_cast<double>(v) * s));
        return r < 1 ? 1U : (r > limit ? limit : r);
    };
    return {.width = side(in.width, frame.width), .height = side(in.height, frame.height)};
}

} // namespace enh::bench
