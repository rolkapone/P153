#include "enh/bench/report.hpp"

#include <nlohmann/json.hpp>

#include <cmath>

namespace enh::bench {

namespace {

using Json = nlohmann::ordered_json;

double round(double v, int digits) {
    const double f = std::pow(10.0, digits);
    return std::round(v * f) / f;
}

double mib(double v) {
    return round(v, 1);
}

Json orNull(const std::string& s) {
    return s.empty() ? Json(nullptr) : Json(s);
}

} // namespace

double fpsOut(std::uint64_t frames, double wall_s) noexcept {
    return wall_s > 0.0 ? static_cast<double>(frames) / wall_s : 0.0;
}

std::string toJsonText(const BenchReport& r) {
    const BenchArgs& p = r.params;
    const bool decode = p.stage == Stage::Decode;
    Json j;
    j["schema_version"] = kSchemaVersion;
    j["tool"] = "enhance-bench";
    j["tool_version"] = r.tool_version;
    j["git_sha"] = orNull(r.git_sha);
    j["gpu"] = orNull(r.gpu);
    j["driver_version"] = orNull(r.driver_version);
    j["cuda_driver_api"] = r.cuda_driver_api;
    j["stage"] = toString(p.stage);
    j["input"] = {{"name", r.input.name},           {"size_bytes", r.input.size_bytes},
                  {"codec", orNull(r.input.codec)}, {"width", r.input.width},
                  {"height", r.input.height},       {"pix_fmt", orNull(r.input.pix_fmt)}};
    j["output"] = decode ? Json(nullptr) : Json({{"name", orNull(r.output_name)}});

    Json params;
    params["repeat"] = p.repeat;
    params["warmup"] = p.warmup;
    params["sample_ms"] = p.sample_ms;
    if (decode) {
        params["decoder"] = toString(p.decoder);
        params["max_frames"] = p.max_frames == 0 ? Json(nullptr) : Json(p.max_frames);
        params["q"] = nullptr;
        params["bits"] = nullptr;
        params["size"] = nullptr;
        params["iterations"] = nullptr;
    } else {
        params["decoder"] = nullptr;
        params["max_frames"] = nullptr;
        params["q"] = p.quality;
        params["bits"] = p.bits;
        params["size"] = std::to_string(p.size.width) + "x" + std::to_string(p.size.height);
        params["iterations"] = p.iterations;
    }
    j["params"] = params;
    j["decoder_path"] = orNull(r.decoder_path);
    j["vsr_feature"] = orNull(r.vsr_feature);

    j["frames_in"] = r.frames_in;
    j["frames_warmup"] = r.frames_warmup;
    j["frames_out"] = r.frames_out;
    j["frames_per_pass"] = r.frames_per_pass;
    j["wall_s"] = round(r.wall_s, 6);
    j["fps_out"] = round(fpsOut(r.frames_out, r.wall_s), 2);

    Json stages = Json::array();
    for (const StageTime& s : r.stages) {
        const double per = s.frames > 0 ? s.ms_total / static_cast<double>(s.frames) : s.ms_total;
        stages.push_back({{"name", s.name},
                          {"ms_total", round(s.ms_total, 3)},
                          {"ms_per_frame", round(per, 4)}});
    }
    j["stages"] = stages;

    j["vram_process_start_mib"] = mib(r.vram_process_start_mib);
    j["vram_context_mib"] = mib(r.vram_context_mib);
    j["vram_rest_mib"] = mib(r.vram_rest_mib);
    j["vram_peak_mib"] = mib(r.vram_peak_mib);
    j["vram_stage_mib"] =
        mib(r.vram_peak_mib > r.vram_rest_mib ? r.vram_peak_mib - r.vram_rest_mib : 0.0);
    j["vram_io_mib"] = decode ? Json(nullptr) : Json(mib(r.vram_io_mib));
    j["vram_device_used_rest_mib"] = mib(r.vram_device_used_rest_mib);
    j["vram_device_used_peak_mib"] = mib(r.vram_device_used_peak_mib);
    j["vram_device_stage_mib"] = mib(r.vram_device_used_peak_mib > r.vram_device_used_rest_mib
                                         ? r.vram_device_used_peak_mib - r.vram_device_used_rest_mib
                                         : 0.0);
    j["vram_drift_mib"] =
        r.repeats.empty() ? 0.0
                          : mib(r.repeats.back().vram_after_mib - r.repeats.front().vram_after_mib);
    Json repeats = Json::array();
    for (const RepeatResult& rep : r.repeats) {
        repeats.push_back({{"frames_out", rep.frames_out},
                           {"wall_s", round(rep.wall_s, 6)},
                           {"fps_out", round(fpsOut(rep.frames_out, rep.wall_s), 2)},
                           {"vram_peak_mib", mib(rep.vram_peak_mib)},
                           {"vram_after_mib", mib(rep.vram_after_mib)}});
    }
    j["repeats"] = repeats;
    j["process_private_mib"] = {{"start", mib(r.private_start_mib)},
                                {"end", mib(r.private_end_mib)}};
    j["sample_period_ms"] = p.sample_ms;
    j["vram_samples"] = r.vram_samples;
    j["warnings"] = r.warnings;
    j["exit_code"] = toInt(r.exit_code);
    if (r.error) {
        j["error"] = {{"code", r.error->code},
                      {"message", r.error->message},
                      {"detail", r.error->detail},
                      {"native_code", r.error->native_code}};
    } else {
        j["error"] = nullptr;
    }
    // Remplacement des sequences UTF-8 invalides : le rapport reste lisible quoi qu'il arrive.
    return j.dump(2, ' ', false, nlohmann::json::error_handler_t::replace) + "\n";
}

} // namespace enh::bench
