// enhance-bench : banc de mesure en ligne de commande pour la QA (ENH-047, ADR-037). Outil de
// developpement, jamais livre. Ce fichier ne fait qu'assembler : arguments, fichiers de sortie,
// contexte CUDA, etage mesure, rapport JSON. Codes de sortie : enh/bench/exit_code.hpp.

#include "enh/bench/args.hpp"
#include "enh/bench/exit_code.hpp"
#include "enh/bench/file_io.hpp"
#include "enh/bench/report.hpp"
#include "enh/pipeline/gpu/cuda_raii.hpp"

#include "gpu/gpu_env.hpp"
#include "gpu/stages.hpp"

#include <array>
#include <cstdio>
#include <exception>
#include <optional>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {

using namespace enh::bench;
namespace pipeline = enh::pipeline;

void printErr(const std::string& text) {
    std::fputs(text.c_str(), stderr);
    std::fputc('\n', stderr);
}

/// Erreur du pipeline -> rapport (code, message francais, detail sans dossier).
void recordError(BenchReport& report, const pipeline::PipelineError& e, const BenchArgs& args) {
    std::vector<std::filesystem::path> paths{args.input};
    if (args.output) {
        paths.push_back(*args.output);
    }
    if (args.report) {
        paths.push_back(*args.report);
    }
    report.exit_code = exitCodeFor(e.code);
    report.error = ErrorInfo{.code = std::string(pipeline::toString(e.code)),
                             .message = std::string(frenchMessage(e.code)),
                             .detail = redactPaths(e.detail, paths),
                             .native_code = e.native_code};
}

void printSummary(const BenchReport& r) {
    if (r.error) {
        printErr("enhance-bench : ECHEC (code " + std::to_string(toInt(r.exit_code)) + ", " +
                 r.error->code + ") " + r.error->message + "\n  detail : " + r.error->detail);
        return;
    }
    std::array<char, 256> line{};
    std::snprintf(line.data(), line.size(),
                  "enhance-bench : %s %s : %llu images en %.3f s = %.1f i/s ; VRAM etage %.0f Mio "
                  "(pic %.0f, repos %.0f)",
                  std::string(toString(r.params.stage)).c_str(), r.input.name.c_str(),
                  static_cast<unsigned long long>(r.frames_out), r.wall_s,
                  fpsOut(r.frames_out, r.wall_s), r.vram_peak_mib - r.vram_rest_mib,
                  r.vram_peak_mib, r.vram_rest_mib);
    printErr(line.data());
}

int run(const std::vector<std::wstring>& argv) {
    auto parsed = parseArgs(argv);
    if (!parsed) {
        printErr("enhance-bench : " + parsed.error());
        printErr("Aide : enhance-bench --help");
        return toInt(ExitCode::InvalidArgs);
    }
    const BenchArgs& args = *parsed;
    if (args.help) {
        std::fputs(std::string(usageText()).c_str(), stdout);
        return toInt(ExitCode::Ok);
    }

    BenchReport report;
    report.params = args;
    report.tool_version = ENH_BENCH_VERSION;
    report.git_sha = ENH_BENCH_GIT_SHA;
    report.input.name = fileNameUtf8(args.input);
    report.private_start_mib = processPrivateMib();

    // Usage VRAM du processus avant toute initialisation CUDA.
    auto probe = gpu::DxgiCudaProbe::createBeforeCuda();
    if (probe) {
        report.vram_process_start_mib = probe->processMib();
    }

    // Fichiers de sortie ouverts tout de suite : un refus coute 0 seconde de mesure.
    std::optional<OutputFile> report_file;
    if (args.report) {
        auto f = OutputFile::create(*args.report, args.force);
        if (!f) {
            printErr("enhance-bench : rapport " + fileNameUtf8(*args.report) + " : " +
                     std::string(frenchMessage(f.error().code)));
            return toInt(exitCodeFor(f.error().code));
        }
        report_file = std::move(*f);
    }
    std::optional<OutputFile> output_file;
    pipeline::Result<void> status{};
    if (args.output) {
        auto f = OutputFile::create(*args.output, args.force);
        if (f) {
            output_file = std::move(*f);
        } else {
            status = enh::Unexpected<pipeline::PipelineError>(std::move(f.error()));
        }
    }

    if (status) {
        status = pipeline::gpu::preloadNvidiaDriverDlls();
    }
    if (status) {
        if (auto ctx = pipeline::gpu::initPrimaryContext(0); !ctx) {
            status = enh::Unexpected<pipeline::PipelineError>(std::move(ctx.error()));
        }
    }
    if (status && !probe) {
        status = pipeline::fail(pipeline::PipelineErrorCode::ComponentMissing,
                                "no NVIDIA adapter visible to DXGI");
    }
    if (status) {
        status = probe->rebindToCudaDevice();
    }
    if (status) {
        report.vram_context_mib = probe->processMib();
        const gpu::GpuInfo info = gpu::queryGpuInfo();
        report.gpu = info.name;
        report.driver_version = info.driver_version;
        report.cuda_driver_api = info.cuda_driver_api;
        status = args.stage == Stage::Decode
                     ? gpu::runDecodeStage(args, *probe, report)
                     : gpu::runVsrImageStage(args, *probe, report, *output_file);
    }
    report.private_end_mib = processPrivateMib();
    if (!status) {
        recordError(report, status.error(), args);
        if (output_file) {
            output_file->discard(); // pas de BMP incomplet
        }
    }

    const std::string json = toJsonText(report);
    if (report_file) {
        if (auto w = report_file->replaceContent(json); !w) {
            printErr("enhance-bench : ecriture du rapport impossible : " +
                     std::string(frenchMessage(w.error().code)));
            return toInt(ExitCode::InternalOrWrite);
        }
    } else {
        std::fputs(json.c_str(), stdout);
    }
    printSummary(report);
    return toInt(report.exit_code);
}

} // namespace

// NOLINTNEXTLINE(misc-use-internal-linkage) : point d entree du programme (liaison externe requise)
int wmain(int argc, wchar_t** argv) {
    SetConsoleOutputCP(CP_UTF8);
    try {
        std::vector<std::wstring> args;
        args.reserve(argc > 0 ? static_cast<std::size_t>(argc) : 0U);
        for (int i = 1; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return run(args);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "enhance-bench : erreur interne : %s\n", e.what());
    } catch (...) {
        std::fputs("enhance-bench : erreur interne inconnue\n", stderr);
    }
    return toInt(ExitCode::InternalOrWrite);
}
