#pragma once

// Etages mesures par enhance-bench v1 (docs/gpu/ENH-047.md). Le contexte CUDA primaire est deja
// cree ; chaque fonction remplit le rapport, meme partiellement en cas d'erreur.

#include "enh/bench/args.hpp"
#include "enh/bench/file_io.hpp"
#include "enh/bench/report.hpp"
#include "enh/pipeline/error.hpp"

#include "gpu_env.hpp"

namespace enh::bench::gpu {

/// --stage decode : demux FFmpeg + NVDEC (ou repli logiciel) d'ENH-008, images en memoire GPU.
[[nodiscard]] pipeline::Result<void> runDecodeStage(const BenchArgs& args, DxgiCudaProbe& probe,
                                                    BenchReport& report);

/// --stage vsr-image : BMP -> CUarray ARGB8 / ABGR10 -> RTX Video (ENH-040) -> BMP 24 bits.
[[nodiscard]] pipeline::Result<void> runVsrImageStage(const BenchArgs& args, DxgiCudaProbe& probe,
                                                      BenchReport& report, OutputFile& output);

} // namespace enh::bench::gpu
