// ENH-047 : rapport JSON (schema v1) conforme au fichier de reference, champs exiges par le
// protocole QA § 4.3, coherence fps_out = frames_out / wall_s, aucun chemin complet.
#include "enh/bench/report.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iterator>
#include <string>

namespace enh::bench {
namespace {

using Json = nlohmann::json;

BenchReport sampleDecodeReport() {
    BenchReport r;
    r.tool_version = "0.1.0";
    r.git_sha = "abcdef1";
    r.gpu = "NVIDIA GeForce RTX 5070";
    r.driver_version = "617.42";
    r.cuda_driver_api = 13040;
    r.params.stage = Stage::Decode;
    r.params.input = L"D:\\Archives\\clips\\BBB-FAST.mp4"; // chemin complet : jamais dans le JSON
    r.params.repeat = 2;
    r.input = {.name = "BBB-FAST.mp4",
               .size_bytes = 6530288,
               .codec = "h264",
               .width = 1280,
               .height = 720,
               .pix_fmt = "nv12"};
    r.decoder_path = "nvdec";
    r.frames_in = 600;
    r.frames_warmup = 60;
    r.frames_out = 540;
    r.frames_per_pass = 300;
    r.frames_expected = 300;
    r.wall_s = 0.18;
    r.stages = {{.name = "demux", .ms_total = 12.5, .frames = 540},
                {.name = "decode", .ms_total = 160.25, .frames = 540}};
    r.vram_process_start_mib = 0.0;
    r.vram_context_mib = 141.04;
    r.vram_rest_mib = 141.0;
    r.vram_peak_mib = 302.46;
    r.vram_device_used_rest_mib = 2300.0;
    r.vram_device_used_peak_mib = 2461.5;
    r.vram_samples = 42;
    r.repeats = {
        {.frames_out = 270, .wall_s = 0.09, .vram_peak_mib = 302.46, .vram_after_mib = 141.0},
        {.frames_out = 270, .wall_s = 0.09, .vram_peak_mib = 300.0, .vram_after_mib = 141.5}};
    r.private_start_mib = 18.0;
    r.private_end_mib = 25.26;
    return r;
}

std::string readText(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

TEST(BenchReport, Decode_IdentiqueAuFichierDeReference) {
    const std::string text = toJsonText(sampleDecodeReport());
    const std::filesystem::path ref =
        std::filesystem::path(ENH_BENCH_TEST_DATA) / "report_decode_v1.json";
    const std::string expected = readText(ref);
    ASSERT_FALSE(expected.empty()) << "reference absente : " << ref.string();
    EXPECT_EQ(Json::parse(text), Json::parse(expected)) << text;
}

TEST(BenchReport, ChampsDuProtocoleQa_Presents) {
    const Json j = Json::parse(toJsonText(sampleDecodeReport()));
    for (const char* key :
         {"frames_in", "frames_out", "wall_s", "fps_out", "stages", "vram_peak_mib",
          "vram_rest_mib", "vram_stage_mib", "process_private_mib", "exit_code", "error",
          "vram_device_used_peak_mib", "vram_drift_mib", "repeats", "sample_period_ms"}) {
        EXPECT_TRUE(j.contains(key)) << key;
    }
    for (const auto& s : j["stages"]) {
        EXPECT_TRUE(s.contains("name") && s.contains("ms_total") && s.contains("ms_per_frame"));
    }
    EXPECT_TRUE(j["process_private_mib"].contains("start"));
    EXPECT_TRUE(j["process_private_mib"].contains("end"));
}

TEST(BenchReport, FpsOut_CoherentAUnPourCent) {
    const Json j = Json::parse(toJsonText(sampleDecodeReport()));
    const double fps = j["fps_out"].get<double>();
    const double expected = j["frames_out"].get<double>() / j["wall_s"].get<double>();
    EXPECT_NEAR(fps, expected, expected * 0.01);
    for (const auto& rep : j["repeats"]) {
        const double e = rep["frames_out"].get<double>() / rep["wall_s"].get<double>();
        EXPECT_NEAR(rep["fps_out"].get<double>(), e, e * 0.01);
    }
}

TEST(BenchReport, AucunCheminComplet) {
    const std::string text = toJsonText(sampleDecodeReport());
    EXPECT_EQ(text.find("Users"), std::string::npos);
    EXPECT_EQ(text.find(":\\\\"), std::string::npos);
    EXPECT_NE(text.find("\"BBB-FAST.mp4\""), std::string::npos);
}

TEST(BenchReport, Echec_ErreurEtCodeDeSortie) {
    BenchReport r;
    r.params.stage = Stage::VsrImage;
    r.input.name = "absent.bmp";
    r.exit_code = ExitCode::InputUnreadable;
    r.error = ErrorInfo{.code = "SourceMissing",
                        .message = "Le fichier d'entree est introuvable.",
                        .detail = "open failed [src/x.cpp:1]",
                        .native_code = 2};
    const Json j = Json::parse(toJsonText(r));
    EXPECT_EQ(j["exit_code"], 2);
    EXPECT_EQ(j["error"]["code"], "SourceMissing");
    EXPECT_EQ(j["error"]["native_code"], 2);
    EXPECT_EQ(j["fps_out"], 0.0);
    EXPECT_EQ(j["stage"], "vsr-image");
    EXPECT_EQ(j["params"]["q"], 4);
    EXPECT_TRUE(j["params"]["decoder"].is_null());
    EXPECT_TRUE(j["vram_io_mib"].is_number());
}

TEST(BenchReport, Utf8InvalideRemplace_PasDException) {
    BenchReport r;
    r.input.name = std::string("bad\xFF\xFEname.mp4");
    std::string text;
    EXPECT_NO_THROW(text = toJsonText(r));
    EXPECT_NO_THROW((void)Json::parse(text));
}

TEST(BenchExitCode, CorrespondanceDesErreurs) {
    using pipeline::PipelineErrorCode;
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::InvalidParams), ExitCode::InvalidArgs);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::OutputExists), ExitCode::InvalidArgs);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::SourceMissing), ExitCode::InputUnreadable);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::SourceCorrupt), ExitCode::InputUnreadable);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::UnsupportedFormat), ExitCode::InputUnreadable);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::SourceUnreadable), ExitCode::InputUnreadable);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::ComponentMissing), ExitCode::GpuUnavailable);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::DeviceLost), ExitCode::GpuUnavailable);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::OutOfVram), ExitCode::OutOfVram);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::Internal), ExitCode::InternalOrWrite);
    EXPECT_EQ(exitCodeFor(PipelineErrorCode::WriteDenied), ExitCode::InternalOrWrite);
    EXPECT_EQ(toInt(ExitCode::OutOfVram), 4);
    EXPECT_FALSE(frenchMessage(PipelineErrorCode::OutOfVram).empty());
}

} // namespace
} // namespace enh::bench
