// SEC-01 / ENH-040 / ENH-041 : controle statique (CI) de la neutralisation de la mise a jour NGX.
// Parcourt src/ et cmake/ : aucune fonction de mise a jour de NGX, aucune Init par identifiant de
// projet, aucune description de projet, aucune DLL `dev` du SDK deployee ; un seul appel a
// NVSDK_NGX_CUDA_Init, avec l'identifiant d'application 0.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct SourceFile {
    fs::path path;
    std::string text;
};

std::vector<SourceFile> sources() {
    std::vector<SourceFile> out;
    for (const char* sub : {"src", "cmake"}) {
        const fs::path root = fs::path(ENH_SOURCE_DIR) / sub;
        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            const fs::path& p = entry.path();
            const std::string ext = p.extension().string();
            if (!entry.is_regular_file() ||
                (ext != ".cpp" && ext != ".hpp" && ext != ".h" && ext != ".cu" && ext != ".cmake" &&
                 p.filename() != "CMakeLists.txt" && ext != ".def")) {
                continue;
            }
            std::ifstream in(p, std::ios::binary);
            out.push_back({.path = p, .text = std::string(std::istreambuf_iterator<char>(in), {})});
        }
    }
    return out;
}

std::size_t countOf(const std::string& text, const std::string& needle) {
    std::size_t n = 0;
    for (auto pos = text.find(needle); pos != std::string::npos;
         pos = text.find(needle, pos + needle.size())) {
        ++n;
    }
    return n;
}

TEST(NgxStaticPolicy, AucuneMiseAJourNiIdentifiantDeProjet) {
    const auto files = sources();
    ASSERT_FALSE(files.empty());
    // Construits par morceaux : ce fichier de test ne doit pas lui-meme contenir les motifs.
    const std::vector<std::string> forbidden = {
        std::string("NVSDK_NGX_") + "UpdateFeature",
        std::string("_Init_") + "with_ProjectID",
        std::string("NVSDK_NGX_") + "ProjectIdDescription",
        std::string("NVSDK_NGX_Application_") + "Identifier_Type_Project_Id",
        std::string("x64/") + "dev",
        std::string("x64\\\\") + "dev",
    };
    for (const SourceFile& f : files) {
        for (const std::string& token : forbidden) {
            EXPECT_EQ(f.text.find(token), std::string::npos)
                << token << " trouve dans " << fs::relative(f.path, ENH_SOURCE_DIR).string();
        }
    }
}

TEST(NgxStaticPolicy, UnSeulInitCudaAvecIdentifiantZero) {
    std::size_t init_calls = 0;
    std::size_t zero_id = 0;
    for (const SourceFile& f : sources()) {
        init_calls += countOf(f.text, "NVSDK_NGX_CUDA_Init(kApplicationId,");
        // Toute autre forme d'appel (identifiant en dur, variante _Ext) est interdite.
        EXPECT_EQ(countOf(f.text, "NVSDK_NGX_CUDA_Init(") -
                      countOf(f.text, "NVSDK_NGX_CUDA_Init(kApplicationId,"),
                  0U)
            << fs::relative(f.path, ENH_SOURCE_DIR).string();
        EXPECT_EQ(countOf(f.text, std::string("NVSDK_NGX_CUDA_") + "Init_Ext"), 0U);
        zero_id += countOf(f.text, "constexpr unsigned long long kApplicationId = 0;");
    }
    EXPECT_EQ(init_calls, 1U);
    EXPECT_EQ(zero_id, 1U);
}

} // namespace
