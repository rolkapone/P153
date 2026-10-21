// ADR-049 : ouverture Win32 sur chemin etendu (> 260 caracteres, accents, emoji, espaces, UNC),
// erreurs precises (SourceMissing / SourceUnreadable), lecture, deplacement, taille.
#include "enh/pipeline/core/win32_file.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline {
namespace {

namespace fs = std::filesystem;

/// Dossier temporaire du test, supprime a la fin (chemins etendus).
class TempDir {
public:
    TempDir() {
        root_ = fs::temp_directory_path() /
                (L"enh-win32file-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                 std::to_wstring(GetTickCount64()));
        CreateDirectoryW(toExtendedPath(root_).c_str(), nullptr);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(fs::path(toExtendedPath(root_)), ec);
    }
    [[nodiscard]] const fs::path& path() const { return root_; }

private:
    fs::path root_;
};

void writeFile(const fs::path& p, const std::string& content) {
    HANDLE h = CreateFileW(toExtendedPath(p).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(h, INVALID_HANDLE_VALUE) << GetLastError();
    DWORD written = 0;
    WriteFile(h, content.data(), static_cast<DWORD>(content.size()), &written, nullptr);
    CloseHandle(h);
}

std::string readAll(Win32File& f) {
    std::string out;
    std::array<std::byte, 7> buf{}; // petit tampon : plusieurs lectures
    for (;;) {
        auto n = f.read(buf);
        EXPECT_TRUE(n);
        if (!n || *n == 0) {
            break;
        }
        out.append(reinterpret_cast<const char*>(buf.data()), *n);
    }
    return out;
}

TEST(Win32File, CheminEtendu) {
    EXPECT_EQ(toExtendedPath(LR"(C:\a\..\b\c.mp4)"), LR"(\\?\C:\b\c.mp4)");
    EXPECT_EQ(toExtendedPath(LR"(\\srv\share\v.mkv)"), LR"(\\?\UNC\srv\share\v.mkv)");
    EXPECT_EQ(toExtendedPath(LR"(\\?\D:\x.mp4)"), LR"(\\?\D:\x.mp4)");
    EXPECT_EQ(toExtendedPath(L"C:/x/y.mp4"), LR"(\\?\C:\x\y.mp4)");
}

TEST(Win32File, LectureDeplacementTaille) {
    TempDir dir;
    const fs::path p = dir.path() / L"data.bin";
    writeFile(p, "0123456789abcdefghij");
    auto f = Win32File::openRead(p);
    ASSERT_TRUE(f) << f.error().detail;
    EXPECT_EQ(*f->size(), 20);
    EXPECT_EQ(readAll(*f), "0123456789abcdefghij");
    EXPECT_EQ(*f->seek(5, SeekOrigin::Begin), 5);
    std::array<std::byte, 3> b{};
    ASSERT_EQ(*f->read(b), 3U);
    EXPECT_EQ(static_cast<char>(b[0]), '5');
    EXPECT_EQ(*f->seek(-2, SeekOrigin::End), 18);
    EXPECT_EQ(*f->seek(1, SeekOrigin::Current), 19);
    EXPECT_EQ(*f->read(b), 1U);
    EXPECT_EQ(*f->read(b), 0U); // fin du fichier
}

TEST(Win32File, NomsUnicodeEspacesEmoji) {
    TempDir dir;
    for (const wchar_t* name :
         {L"été à Noël.mp4", L"日本語 & espace.mkv", L"clip \U0001F3AC.mov"}) {
        const fs::path p = dir.path() / name;
        writeFile(p, "abc");
        auto f = Win32File::openRead(p);
        ASSERT_TRUE(f) << f.error().detail;
        EXPECT_EQ(readAll(*f), "abc");
    }
}

TEST(Win32File, CheminDePlusDe260Caracteres) {
    TempDir dir;
    fs::path p = dir.path();
    for (int i = 0; i < 6; ++i) {
        p /= std::wstring(60, static_cast<wchar_t>(L'a' + i));
        ASSERT_TRUE(CreateDirectoryW(toExtendedPath(p).c_str(), nullptr)) << GetLastError();
    }
    p /= L"vidéo longue.mp4";
    ASSERT_GT(p.native().size(), 260U);
    writeFile(p, "long");
    auto f = Win32File::openRead(p); // chemin SANS prefixe : la classe l'ajoute
    ASSERT_TRUE(f) << f.error().detail;
    EXPECT_EQ(readAll(*f), "long");
}

TEST(Win32File, FichierAbsent_SourceMissing) {
    TempDir dir;
    auto f = Win32File::openRead(dir.path() / L"absent.mp4");
    ASSERT_FALSE(f);
    EXPECT_EQ(f.error().code, PipelineErrorCode::SourceMissing);
    EXPECT_EQ(f.error().native_code, ERROR_FILE_NOT_FOUND);
    auto g = Win32File::openRead(dir.path() / L"pas-de-dossier" / L"x.mp4");
    ASSERT_FALSE(g);
    EXPECT_EQ(g.error().code, PipelineErrorCode::SourceMissing);
}

TEST(Win32File, FichierVerrouille_SourceUnreadable) {
    TempDir dir;
    const fs::path p = dir.path() / L"verrou.mp4";
    writeFile(p, "x");
    HANDLE lock = CreateFileW(toExtendedPath(p).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(lock, INVALID_HANDLE_VALUE);
    auto f = Win32File::openRead(p);
    CloseHandle(lock);
    ASSERT_FALSE(f);
    EXPECT_EQ(f.error().code, PipelineErrorCode::SourceUnreadable);
    EXPECT_EQ(f.error().native_code, ERROR_SHARING_VIOLATION);
}

TEST(Win32File, DossierRefuse) {
    TempDir dir;
    auto f = Win32File::openRead(dir.path());
    ASSERT_FALSE(f);
    EXPECT_EQ(f.error().code, PipelineErrorCode::SourceUnreadable);
}

TEST(Win32File, DeplacementEtFermeture) {
    TempDir dir;
    const fs::path p = dir.path() / L"m.bin";
    writeFile(p, "xyz");
    auto f = Win32File::openRead(p);
    ASSERT_TRUE(f);
    Win32File moved = std::move(*f);
    EXPECT_TRUE(moved.isOpen());
    EXPECT_FALSE(f->isOpen()); // NOLINT(bugprone-use-after-move) : etat apres deplacement teste
    EXPECT_FALSE(f->read(std::span<std::byte>{}));
}

TEST(Win32File, PartageUnc) {
    // Partage local a declarer par ENH_TEST_UNC_DIR (ex. \\localhost\partage\dossier).
    std::array<wchar_t, 32768> buf{};
    const DWORD n =
        GetEnvironmentVariableW(L"ENH_TEST_UNC_DIR", buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0 || n >= buf.size()) {
        GTEST_SKIP() << "ENH_TEST_UNC_DIR non defini";
    }
    const std::wstring unc(buf.data(), n);
    const fs::path p = fs::path(unc) / L"enh-unc-é.bin";
    writeFile(p, "unc");
    auto f = Win32File::openRead(p);
    ASSERT_TRUE(f) << f.error().detail;
    EXPECT_EQ(readAll(*f), "unc");
    f = Win32File::openRead(fs::path(unc) / L"absent.bin");
    DeleteFileW(toExtendedPath(p).c_str());
}

} // namespace
} // namespace enh::pipeline
