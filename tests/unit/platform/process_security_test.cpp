#include "enh/platform/process_security.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace {

using enh::platform::classifyModulePath;
using enh::platform::ModuleOrigin;
using enh::platform::TrustedModuleRoots;

// Chemins Windows en chaines brutes R"(...)" : pas d'echappement des barres obliques inverses.
TrustedModuleRoots sampleRoots() {
    return TrustedModuleRoots{LR"(D:\Apps\Enhance)",
                              LR"(C:\Windows\System32)",
                              LR"(C:\Windows\WinSxS)",
                              {LR"(C:\Program Files)"}};
}

} // namespace

TEST(ModulePaths, DossierApplicationEtSystemeDeConfiance) {
    const auto roots = sampleRoots();
    EXPECT_EQ(classifyModulePath(LR"(D:\Apps\Enhance\Qt6Core.dll)", roots), ModuleOrigin::Trusted);
    EXPECT_EQ(classifyModulePath(LR"(D:\Apps\Enhance\qml\QtQuick\qtquick2plugin.dll)", roots),
              ModuleOrigin::Trusted);
    EXPECT_EQ(classifyModulePath(LR"(c:\windows\SYSTEM32\kernel32.DLL)", roots),
              ModuleOrigin::Trusted);
    EXPECT_EQ(classifyModulePath(
                  LR"(C:\Windows\System32\DriverStore\FileRepository\nv.inf\nvldumdx.dll)", roots),
              ModuleOrigin::Trusted);
    EXPECT_EQ(classifyModulePath(LR"(\\?\C:\Windows\WinSxS\amd64_x\comctl32.dll)", roots),
              ModuleOrigin::Trusted);
}

TEST(ModulePaths, PrefixeTrompeurRefuse) {
    const auto roots = sampleRoots();
    // « D:\Apps\Enhance-pirate » commence comme le dossier de l'application sans y etre.
    EXPECT_EQ(classifyModulePath(LR"(D:\Apps\Enhance-pirate\Qt6Core.dll)", roots),
              ModuleOrigin::Untrusted);
    EXPECT_EQ(classifyModulePath(LR"(C:\Windows\System32evil\x.dll)", roots),
              ModuleOrigin::Untrusted);
}

TEST(ModulePaths, RemonteeParPointPointRefusee) {
    const auto roots = sampleRoots();
    EXPECT_EQ(classifyModulePath(LR"(D:\Apps\Enhance\..\Autre\evil.dll)", roots),
              ModuleOrigin::Untrusted);
}

TEST(ModulePaths, DossiersInscriptiblesRefuses) {
    const auto roots = sampleRoots();
    EXPECT_EQ(classifyModulePath(LR"(D:\Telechargements\version.dll)", roots),
              ModuleOrigin::Untrusted);
    EXPECT_EQ(classifyModulePath(LR"(C:\Windows\Temp\evil.dll)", roots), ModuleOrigin::Untrusted);
}

TEST(ModulePaths, LogicielInstalleSignaleSansEchec) {
    const auto roots = sampleRoots();
    EXPECT_EQ(classifyModulePath(LR"(C:\Program Files\Antivirus\hook.dll)", roots),
              ModuleOrigin::ProtectedThirdParty);
    EXPECT_FALSE(
        enh::platform::isTrustedModulePath(LR"(C:\Program Files\Antivirus\hook.dll)", roots));
}

TEST(ModulePaths, CheminsUnicodeEtLongs) {
    const TrustedModuleRoots roots{
        L"D:\\Vid\u00e9os\\Enhance \u2728", LR"(C:\Windows\System32)", {}, {}};
    std::wstring deep = L"D:\\Vid\u00e9os\\Enhance \u2728";
    for (int i = 0; i < 40; ++i) {
        deep += LR"(\sous-dossier-long)";
    }
    EXPECT_GT(deep.size(), 260U);
    EXPECT_EQ(classifyModulePath(deep + LR"(\x.dll)", roots), ModuleOrigin::Trusted);
}

TEST(ProcessSecurity, ModulesDuProcessusCourantEnumeres) {
    const auto roots = enh::platform::currentTrustedModuleRoots();
    EXPECT_FALSE(roots.applicationDir.empty());
    EXPECT_FALSE(roots.system32Dir.empty());
    const auto modules = enh::platform::loadedModulePaths();
    ASSERT_TRUE(modules.has_value()) << "liste des modules incomplete";
    ASSERT_FALSE(modules->empty());
    const auto trusted = std::ranges::count_if(*modules, [&](const auto& m) {
        return classifyModulePath(m, roots) == ModuleOrigin::Trusted;
    });
    EXPECT_GT(trusted, 0); // au moins l'executable et kernel32
}

TEST(ProcessSecurity, ElevationLisible) {
    // Valeur dependante de la session : on verifie seulement que l'appel aboutit.
    [[maybe_unused]] const bool elevated = enh::platform::isProcessElevated();
    SUCCEED();
}
