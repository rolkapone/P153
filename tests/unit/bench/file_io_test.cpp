// ENH-047 : fichiers du banc (CREATE_NEW, --force, suppression d'une sortie incomplete, lecture
// bornee) et retrait des dossiers dans les messages du JSON.
#include "enh/bench/file_io.hpp"
#include "enh/pipeline/core/win32_file.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>

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
using pipeline::PipelineErrorCode;

class BenchFileIo : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = fs::temp_directory_path() /
               (L"enh-bench-io-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                std::to_wstring(GetTickCount64())) /
               L"dossier é 日本";
        fs::create_directories(dir_);
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(dir_.parent_path(), ec);
    }
    static std::string readAll(const fs::path& p) {
        std::ifstream in(p, std::ios::binary);
        return {std::istreambuf_iterator<char>(in), {}};
    }
    fs::path dir_;
};

TEST_F(BenchFileIo, CreateNew_RefuseUnFichierExistant_SaufForce) {
    const fs::path p = dir_ / L"rapport.json";
    {
        auto f = OutputFile::create(p, false);
        ASSERT_TRUE(f) << f.error().detail;
        ASSERT_TRUE(f->replaceContent(std::string_view("premier contenu long")));
        ASSERT_TRUE(f->replaceContent(std::string_view("court"))); // fin du fichier recalee
        f->commit();
    }
    EXPECT_EQ(readAll(p), "court");
    auto again = OutputFile::create(p, false);
    ASSERT_FALSE(again);
    EXPECT_EQ(again.error().code, PipelineErrorCode::OutputExists);
    EXPECT_EQ(readAll(p), "court") << "le fichier existant ne doit pas etre touche";
    {
        auto forced = OutputFile::create(p, true);
        ASSERT_TRUE(forced) << forced.error().detail;
        ASSERT_TRUE(forced->replaceContent(std::string_view("ecrase")));
        forced->commit();
    }
    EXPECT_EQ(readAll(p), "ecrase");
}

TEST_F(BenchFileIo, Discard_SupprimeLaSortieIncomplete) {
    const fs::path p = dir_ / L"sortie.bmp";
    auto f = OutputFile::create(p, false);
    ASSERT_TRUE(f);
    ASSERT_TRUE(f->replaceContent(std::string_view("partiel")));
    f->discard();
    EXPECT_FALSE(fs::exists(p));
    f->discard(); // sans effet une seconde fois
    EXPECT_FALSE(f->replaceContent(std::string_view("x")));
}

TEST_F(BenchFileIo, SansCommit_SortieSupprimee_MemeSurException) {
    const fs::path p = dir_ / L"rapport.json";
    try {
        auto f = OutputFile::create(p, false);
        ASSERT_TRUE(f);
        ASSERT_TRUE(f->replaceContent(std::string_view("a moitie ecrit")));
        throw std::runtime_error("echec simule pendant la mesure");
    } catch (const std::runtime_error&) { // NOLINT(bugprone-empty-catch) : exception attendue
    }
    EXPECT_FALSE(fs::exists(p)) << "sortie incomplete laissee apres une exception";
    {
        auto f = OutputFile::create(p, false);
        ASSERT_TRUE(f);
    } // ni ecriture ni commit : pas de fichier de 0 octet
    EXPECT_FALSE(fs::exists(p));
}

TEST_F(BenchFileIo, Commit_GardeLaSortie_DeplacementSansDoubleSuppression) {
    const fs::path p = dir_ / L"sortie.bmp";
    std::optional<OutputFile> holder;
    {
        auto f = OutputFile::create(p, false);
        ASSERT_TRUE(f);
        holder = std::move(*f); // l'objet source deplace ne supprime rien en mourant
    }
    EXPECT_TRUE(fs::exists(p));
    ASSERT_TRUE(holder->replaceContent(std::string_view("complet")));
    holder->commit();
    holder.reset();
    EXPECT_EQ(readAll(p), "complet");
}

TEST_F(BenchFileIo, SameFile_CheminsEquivalents) {
    const fs::path a = dir_ / L"a.bmp";
    {
        std::ofstream(a, std::ios::binary) << "x";
    }
    EXPECT_TRUE(sameFile(a, dir_ / L"." / L"a.bmp"));
    EXPECT_TRUE(sameFile(a, dir_ / L"A.BMP"));
    EXPECT_TRUE(sameFile(a, dir_ / L"sous" / L".." / L"a.bmp"));
    EXPECT_FALSE(sameFile(a, dir_ / L"b.bmp"));
    // Deux sorties inexistantes : comparaison des chemins normalises.
    EXPECT_TRUE(sameFile(dir_ / L"n.json", dir_ / L"." / L"N.json"));
    EXPECT_FALSE(sameFile(dir_ / L"n.json", dir_ / L"m.json"));
    // Lien physique : meme fichier sous un autre nom.
    const fs::path link = dir_ / L"lien.bmp";
    std::error_code ec;
    fs::create_hard_link(a, link, ec);
    if (!ec) {
        EXPECT_TRUE(sameFile(a, link));
    }
}

TEST_F(BenchFileIo, CheminsRelatifs_PointBarreContreNomSeul) {
    {
        std::ofstream(dir_ / L"a.bmp", std::ios::binary) << "x";
    }
    const fs::path previous = fs::current_path();
    fs::current_path(dir_);
    const bool same = sameFile(LR"(.\a.bmp)", L"a.bmp");
    const auto check = checkDistinctFiles(L"a.bmp", fs::path(LR"(.\a.bmp)"), std::nullopt);
    fs::current_path(previous);
    EXPECT_TRUE(same);
    ASSERT_FALSE(check);
    EXPECT_NE(check.error().find("--out designe le fichier d'entree"), std::string::npos);
}

TEST_F(BenchFileIo, CheckDistinctFiles_RefusMemeAvecForce) {
    const fs::path in = dir_ / L"in.bmp";
    {
        std::ofstream(in, std::ios::binary) << "entree";
    }
    EXPECT_TRUE(checkDistinctFiles(in, dir_ / L"out.bmp", dir_ / L"r.json"));
    EXPECT_TRUE(checkDistinctFiles(in, std::nullopt, std::nullopt));
    auto out_in = checkDistinctFiles(in, dir_ / L"." / L"IN.bmp", std::nullopt);
    ASSERT_FALSE(out_in);
    auto rep_in = checkDistinctFiles(in, std::nullopt, dir_ / L"in.bmp");
    ASSERT_FALSE(rep_in);
    EXPECT_NE(rep_in.error().find("--report designe le fichier d'entree"), std::string::npos);
    auto out_rep = checkDistinctFiles(in, dir_ / L"o.bmp", dir_ / L"x" / L".." / L"o.bmp");
    ASSERT_FALSE(out_rep);
    EXPECT_EQ(readAll(in), "entree") << "l'entree ne doit jamais etre touchee";
}

TEST_F(BenchFileIo, Create_DossierAbsent_WriteDenied) {
    auto f = OutputFile::create(dir_ / L"absent" / L"r.json", false);
    ASSERT_FALSE(f);
    EXPECT_EQ(f.error().code, PipelineErrorCode::WriteDenied);
}

TEST_F(BenchFileIo, LectureBornee) {
    const fs::path p = dir_ / L"image.bmp";
    {
        std::ofstream out(p, std::ios::binary);
        out << "0123456789";
    }
    auto ok = readFileBounded(p, 10);
    ASSERT_TRUE(ok) << ok.error().detail;
    EXPECT_EQ(ok->size(), 10U);
    auto big = readFileBounded(p, 9);
    ASSERT_FALSE(big);
    EXPECT_EQ(big.error().code, PipelineErrorCode::UnsupportedFormat);
    const fs::path empty = dir_ / L"vide.bmp";
    {
        std::ofstream out(empty, std::ios::binary);
    }
    auto e = readFileBounded(empty, 10);
    ASSERT_FALSE(e);
    EXPECT_EQ(e.error().code, PipelineErrorCode::SourceCorrupt);
    auto missing = readFileBounded(dir_ / L"absent.bmp", 10);
    ASSERT_FALSE(missing);
    EXPECT_EQ(missing.error().code, PipelineErrorCode::SourceMissing);
}

TEST_F(BenchFileIo, RedactPaths_RetireLesDossiers) {
    const fs::path in = dir_ / L"BBB-FAST.mp4";
    const std::u8string dir8 = dir_.u8string();
    const std::string dir_utf8(dir8.begin(), dir8.end());
    const std::u8string ext8 = fs::path(pipeline::toExtendedPath(in)).u8string();
    const std::string ext_utf8(ext8.begin(), ext8.end());
    const std::string text = "open failed: " + dir_utf8 + "\\BBB-FAST.mp4 / " + ext_utf8;
    const fs::path paths[] = {in};
    const std::string out = redactPaths(text, paths);
    EXPECT_EQ(out.find(dir_utf8), std::string::npos) << out;
    EXPECT_EQ(out.find("Temp"), std::string::npos) << out;
    EXPECT_NE(out.find("<dir>\\BBB-FAST.mp4"), std::string::npos) << out;
    EXPECT_EQ(redactPaths("rien a retirer", paths), "rien a retirer");
}

TEST(BenchFileName, NomSeulEnUtf8) {
    EXPECT_EQ(fileNameUtf8(L"D:\\Videos\\x\\vidéo.mp4"), "vid\xC3\xA9o.mp4");
    EXPECT_EQ(fileNameUtf8(L"clip.mkv"), "clip.mkv");
}

TEST(BenchPrivateMemory, Positive) {
    EXPECT_GT(processPrivateMib(), 0.0);
}

} // namespace
} // namespace enh::bench
