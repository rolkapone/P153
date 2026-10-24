// ENH-047 sur la RTX (local seulement, label gpu) : enhance-bench.exe lance comme la QA le lance
// (processus separe), JSON conforme et coherent, codes de sortie, nombre d'images = ffprobe,
// VRAM d'etage sous les seuils du sprint 1, pas de derive sur --repeat 10, sortie VSR 4K et
// q4 != q0, aucun chemin complet dans le rapport.
// Clips : tests/clips (ENH_CLIPS_DIR pour un autre dossier). Absent = ECHEC.
#include "enh/bench/bmp.hpp"
#include "enh/bench/file_io.hpp"

#include "ffmpeg/clip_writer.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::bench {
namespace {

namespace fs = std::filesystem;
using Json = nlohmann::json;

fs::path clipsDir() {
    std::array<wchar_t, 4096> buf{};
    const DWORD n =
        GetEnvironmentVariableW(L"ENH_CLIPS_DIR", buf.data(), static_cast<DWORD>(buf.size()));
    return n > 0 && n < buf.size() ? fs::path(std::wstring(buf.data(), n))
                                   : fs::path(ENH_CLIPS_DIR);
}

std::wstring quote(const std::wstring& s) {
    std::wstring out = L"\"";
    for (const wchar_t c : s) {
        if (c == L'"') {
            out += L'\\';
        }
        out += c;
    }
    return out + L"\"";
}

/// Lance `exe args...` et attend la fin ; rend le code de sortie (-1 si le lancement echoue).
int runProcess(const fs::path& exe, const std::vector<std::wstring>& args) {
    std::wstring cmd = quote(exe.wstring());
    for (const std::wstring& a : args) {
        cmd += L' ';
        cmd += quote(a);
    }
    STARTUPINFOW si{};
    si.cb = sizeof si;
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(exe.wstring().c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr,
                       nullptr, &si, &pi) == FALSE) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(code);
}

int bench(const std::vector<std::wstring>& args) {
    return runProcess(fs::path(ENH_BENCH_EXE), args);
}

std::string readText(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

Json readJson(const fs::path& p) {
    const std::string t = readText(p);
    EXPECT_FALSE(t.empty()) << "rapport absent";
    return t.empty() ? Json::object() : Json::parse(t);
}

void expectCoherent(const Json& j) {
    for (const char* key :
         {"frames_in", "frames_out", "wall_s", "fps_out", "stages", "vram_peak_mib",
          "vram_rest_mib", "vram_stage_mib", "process_private_mib", "exit_code"}) {
        ASSERT_TRUE(j.contains(key)) << key;
    }
    const double expected = j["frames_out"].get<double>() / j["wall_s"].get<double>();
    EXPECT_NEAR(j["fps_out"].get<double>(), expected, expected * 0.01);
    const std::string text = j.dump();
    EXPECT_EQ(text.find(":\\\\"), std::string::npos) << "chemin complet dans le JSON";
    EXPECT_EQ(text.find("Users"), std::string::npos);
}

class ENH047Bench : public ::testing::Test {
protected:
    pipeline::testing::ScopedTempDir tmp_;
};

TEST_F(ENH047Bench, Decode_BbbFast_JsonCoherent_ImagesEgalFfprobe) {
    const fs::path clip = clipsDir() / L"in720" / L"BBB-FAST.mp4";
    ASSERT_TRUE(fs::exists(clip)) << "clip absent : lancer scripts/fetch-clips.ps1";
    const fs::path rep = tmp_.path() / L"decode.json";
    ASSERT_EQ(bench({L"--stage", L"decode", L"--in", clip.wstring(), L"--report", rep.wstring()}),
              0);
    const Json j = readJson(rep);
    expectCoherent(j);
    const auto ref = pipeline::testing::ffprobeFrameCount(ENH_FFPROBE_EXE, clip);
    ASSERT_TRUE(ref.has_value());
    EXPECT_EQ(j["frames_per_pass"].get<long long>(), *ref);
    EXPECT_EQ(j["frames_in"].get<long long>(), *ref);
    EXPECT_EQ(j["frames_out"].get<long long>(), *ref - 30);
    EXPECT_EQ(j["decoder_path"], "nvdec");
    EXPECT_EQ(j["input"]["name"], "BBB-FAST.mp4");
    EXPECT_LE(j["vram_stage_mib"].get<double>(), 600.0);
    EXPECT_GE(j["fps_out"].get<double>(), 1000.0);
    std::cout << "[ENH-047] decode BBB-FAST.mp4 : " << j["fps_out"] << " i/s, VRAM etage "
              << j["vram_stage_mib"] << " Mio (DXGI), " << j["vram_device_stage_mib"]
              << " Mio (cudaMemGetInfo)\n";
}

TEST_F(ENH047Bench, Decode_Repeat10_SansDerive) {
    const fs::path rep = tmp_.path() / L"repeat.json";
    ASSERT_EQ(
        bench({L"--stage", L"decode", L"--in", (clipsDir() / L"in720" / L"BBB-PAN.mp4").wstring(),
               L"--repeat", L"10", L"--report", rep.wstring()}),
        0);
    const Json j = readJson(rep);
    expectCoherent(j);
    ASSERT_EQ(j["repeats"].size(), 10U);
    std::vector<double> fps;
    for (const auto& r : j["repeats"]) {
        fps.push_back(r["fps_out"].get<double>());
    }
    std::ranges::sort(fps);
    const double spread = (fps.back() - fps.front()) / fps[fps.size() / 2];
    std::cout << "[ENH-047] --repeat 10 : derive VRAM " << j["vram_drift_mib"] << " Mio, ecart i/s "
              << spread * 100.0 << " %\n";
    EXPECT_LE(std::abs(j["vram_drift_mib"].get<double>()), 50.0);
}

TEST_F(ENH047Bench, Decode_RepliLogiciel_Ffv1) {
    const fs::path rep = tmp_.path() / L"ffv1.json";
    ASSERT_EQ(
        bench({L"--stage", L"decode", L"--in", (clipsDir() / L"in720" / L"BBB-FAST.mkv").wstring(),
               L"--report", rep.wstring()}),
        0);
    const Json j = readJson(rep);
    EXPECT_EQ(j["decoder_path"], "software");
    EXPECT_EQ(j["frames_per_pass"], 300);
    // --decoder nvdec : echec au lieu du repli.
    const fs::path rep2 = tmp_.path() / L"ffv1_nvdec.json";
    EXPECT_EQ(
        bench({L"--stage", L"decode", L"--in", (clipsDir() / L"in720" / L"BBB-FAST.mkv").wstring(),
               L"--decoder", L"nvdec", L"--report", rep2.wstring()}),
        2);
    EXPECT_EQ(readJson(rep2)["exit_code"], 2);
}

TEST_F(ENH047Bench, CodesDeSortie_EntreesIllisibles) {
    const fs::path zero = tmp_.path() / L"zero.mp4";
    {
        std::ofstream(zero, std::ios::binary).flush();
    }
    const fs::path rep = tmp_.path() / L"zero.json";
    EXPECT_EQ(bench({L"--stage", L"decode", L"--in", zero.wstring(), L"--report", rep.wstring()}),
              2);
    const Json j = readJson(rep);
    EXPECT_EQ(j["exit_code"], 2);
    EXPECT_FALSE(j["error"].is_null());
    EXPECT_EQ(j["input"]["name"], "zero.mp4");

    const fs::path absent = tmp_.path() / L"absent é.mp4";
    const fs::path rep2 = tmp_.path() / L"absent.json";
    EXPECT_EQ(
        bench({L"--stage", L"decode", L"--in", absent.wstring(), L"--report", rep2.wstring()}), 2);
    const std::string text = readText(rep2);
    EXPECT_EQ(text.find(tmp_.path().filename().string()), std::string::npos)
        << "dossier present dans le JSON : " << text;

    EXPECT_EQ(bench({L"--stage", L"decode"}), 1);
    // Rapport existant : refuse sans --force.
    EXPECT_EQ(bench({L"--stage", L"decode", L"--in", zero.wstring(), L"--report", rep.wstring()}),
              1);
}

TEST_F(ENH047Bench, Decode_VideoCorrompueNonVide_Code2_JsonAvecErreur) {
    // 64 Kio d'octets pseudo-aleatoires sous une extension .mp4 (ni vide, ni conteneur valide).
    const fs::path bad = tmp_.path() / L"corrompue.mp4";
    {
        std::ofstream f(bad, std::ios::binary);
        std::uint32_t x = 2463534242U; // xorshift32, deterministe
        for (int i = 0; i < 65536; ++i) {
            x ^= x << 13U;
            x ^= x >> 17U;
            x ^= x << 5U;
            f.put(static_cast<char>(x & 0xFFU));
        }
    }
    const fs::path rep = tmp_.path() / L"corrompue.json";
    EXPECT_EQ(bench({L"--stage", L"decode", L"--in", bad.wstring(), L"--report", rep.wstring()}),
              2);
    const Json j = readJson(rep);
    EXPECT_EQ(j["exit_code"], 2);
    ASSERT_FALSE(j["error"].is_null());
    EXPECT_FALSE(j["error"]["code"].get<std::string>().empty());
    EXPECT_EQ(j["input"]["size_bytes"], 65536);
}

TEST_F(ENH047Bench, SortieDesignantLEntree_Code1_EntreeIntacte_MemeAvecForce) {
    const fs::path in = tmp_.path() / L"a.bmp";
    auto bmp = encodeBmp24(RgbImage{.size_px = {16, 16}, .rgb = std::vector<std::uint8_t>(768, 7)});
    ASSERT_TRUE(bmp);
    {
        std::ofstream f(in, std::ios::binary);
        f.write(reinterpret_cast<const char*>(bmp->data()),
                static_cast<std::streamsize>(bmp->size()));
    }
    const std::string before = readText(in);
    const fs::path dotted = tmp_.path() / L"." / L"A.bmp";
    EXPECT_EQ(bench({L"--stage", L"vsr-image", L"--in", in.wstring(), L"--out", dotted.wstring(),
                     L"--force"}),
              1);
    EXPECT_EQ(bench({L"--stage", L"decode", L"--in", in.wstring(), L"--report", dotted.wstring(),
                     L"--force"}),
              1);
    EXPECT_EQ(readText(in), before) << "l'entree a ete modifiee";
}

/// Image n de BBB-FAST.mkv (720p, FFV1 sans perte) en BMP 24 bits, par le ffmpeg de la build.
fs::path extractFrame(const fs::path& dir) {
    const fs::path out = dir / L"in720.bmp";
    const int code = runProcess(
        fs::path(ENH_FFMPEG_EXE),
        {L"-v", L"error", L"-y", L"-i", (clipsDir() / L"in720" / L"BBB-FAST.mkv").wstring(), L"-vf",
         L"select=eq(n\\,150)", L"-frames:v", L"1", L"-pix_fmt", L"bgr24", out.wstring()});
    EXPECT_EQ(code, 0) << "extraction de l'image par ffmpeg";
    return out;
}

TEST_F(ENH047Bench, VsrImage_Sortie4K_Q4DiffereDeQ0_VramSousLeSeuil) {
    const fs::path in = extractFrame(tmp_.path());
    ASSERT_TRUE(fs::exists(in));
    std::vector<std::vector<std::uint8_t>> outputs;
    for (const wchar_t* q : {L"0", L"4"}) {
        const fs::path out = tmp_.path() / (std::wstring(L"vsr_q") + q + L".bmp");
        const fs::path rep = tmp_.path() / (std::wstring(L"vsr_q") + q + L".json");
        ASSERT_EQ(
            bench({L"--stage", L"vsr-image", L"--in", in.wstring(), L"--out", out.wstring(), L"--q",
                   q, L"--bits", L"10", L"--iterations", L"60", L"--report", rep.wstring()}),
            0);
        const Json j = readJson(rep);
        expectCoherent(j);
        EXPECT_EQ(j["output"]["name"], fs::path(out).filename().string());
        EXPECT_LE(j["vram_stage_mib"].get<double>(), 300.0);
        std::cout << "[ENH-047] VSR q" << static_cast<char>(*q) << " 10 bits : " << j["fps_out"]
                  << " i/s, VRAM etage " << j["vram_stage_mib"] << " Mio, " << j["vsr_feature"]
                  << "\n";
        auto bytes = readFileBounded(out, kMaxBmpFileBytes);
        ASSERT_TRUE(bytes);
        auto img = decodeBmp(*bytes);
        ASSERT_TRUE(img) << img.error().detail;
        EXPECT_EQ(img->size_px, (pipeline::Size{3840, 2160}));
        outputs.push_back(std::move(img->rgb));
    }
    std::size_t diff = 0;
    for (std::size_t i = 0; i < outputs[0].size(); ++i) {
        diff += outputs[0][i] != outputs[1][i] ? 1U : 0U;
    }
    const double ratio = static_cast<double>(diff) / static_cast<double>(outputs[0].size());
    std::cout << "[ENH-047] octets q4 != q0 : " << ratio * 100.0 << " %\n";
    EXPECT_GE(ratio, 0.30); // seuil QA ENH-040 (image reelle)
}

TEST_F(ENH047Bench, VsrImage_BmpMalforme_Code2_SansSortie) {
    const fs::path bad = tmp_.path() / L"bad.bmp";
    {
        std::ofstream f(bad, std::ios::binary);
        f << "BM not really a bitmap, truncated header";
    }
    const fs::path out = tmp_.path() / L"never.bmp";
    EXPECT_EQ(bench({L"--stage", L"vsr-image", L"--in", bad.wstring(), L"--out", out.wstring()}),
              2);
    EXPECT_FALSE(fs::exists(out)) << "sortie incomplete laissee sur le disque";
}

} // namespace
} // namespace enh::bench
