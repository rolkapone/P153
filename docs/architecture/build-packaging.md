> Extrait de l'ancien `docs/architecture.md` (ménage docs du 2026-10-29), texte inchangé. Index : [architecture/index.md](index.md).

## 15. Build, dépendances, CI, packaging (devops)

Statut : proposition devops du 2026-10-21 (ADR-020 à ADR-037), **à valider par le PO avant toute installation**. Rien n'est encore installé ni téléchargé. **[H]** = hypothèse à vérifier à l'étape 5 (installation) ou en ENH-003.

Principe directeur : **deux mondes de build**.
- **Monde sans GPU** (CI, et tout poste sans SDK) : `ENH_WITH_GPU=OFF`, pipeline simulé (`FakePipeline`), aucune DLL NVIDIA, aucun SDK. C'est ce que la CI compile.
- **Monde GPU** (machine du PO) : `ENH_WITH_GPU=ON`, CUDA Toolkit + SDK dans `third_party/nvidia/`, tests `tests/gpu/`.


### 15.18 Installation : ce que fait le PO, ce que fait devops (étape 5)

**À installer par le PO (comptes, gros téléchargements, choix manuels).** Rien n'est à faire avant validation de cette section.
1. **SDK NVIDIA** (compte NVIDIA Developer) : RTX Video SDK, Video Codec SDK ≥ 13.0, Optical Flow SDK ≥ 5.0, TensorRT 10.x (ZIP Windows pour CUDA 13) à poser dans `F:\P153\third_party\nvidia\<sdk>\` (liens dans `docs/gpu/spike.md` §2.1) ; noter les versions.
2. **CUDA Toolkit 13.x** (version précisée par devops après lecture des notes VS 2026) : installeur NVIDIA, **mode personnalisé, décocher « Driver »** pour garder le pilote 617.42 (`winget show Nvidia.CUDA --versions` liste les versions, mais winget ne permet pas de décocher le pilote : utiliser l'installeur graphique). Vérification : `nvcc --version` et `echo $env:CUDA_PATH`.
3. **Composants Visual Studio 2026** (Visual Studio Installer > Modifier) : « Développement Desktop C++ » (déjà là) + « Outils CMake C++ pour Windows » + « Outils C++ Clang pour Windows » (clang-format/tidy) ; **optionnel (repli ADR-021)** « Outils de build MSVC v143 - VS 2022 C++ x64/x86 ». Vérification : `& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`.
4. **Espace disque** : prévoir ≈ 40 Gio libres (Qt temporaire 25 Gio, caches, SDK, TensorRT ~ 4 Gio).
5. Facultatif (devops peut le faire) : `winget install -e --id Git.Git` si absent.

**Installé/déposé par devops à l'étape 5 (sans compte, après accord du PO).**
- vcpkg : `git clone https://github.com/microsoft/vcpkg C:\Tools\vcpkg` ; `git -C C:\Tools\vcpkg checkout <sha de la baseline>` ; `C:\Tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics` ; variables utilisateur `VCPKG_ROOT=C:\Tools\vcpkg`, `VCPKG_DISABLE_METRICS=1`, `VCPKG_DEFAULT_BINARY_CACHE=F:\vcpkg-cache` (`[Environment]::SetEnvironmentVariable(..., 'User')`).
- Premier `scripts/build.ps1 -Preset dev` : compilation de Qt (≈ 1 h 20), GoogleTest, spdlog, nlohmann-json, tl-expected.
- FFmpeg BtbN `lgpl-shared` : `scripts/fetch-ffmpeg.ps1` (SHA-256 vérifié) vers `third_party/ffmpeg/lgpl-shared/` ; FFmpeg « outil » GPL pour la QA : hors dépôt, avec l'accord du PO.
- LLVM/clang-format à la version épinglée (`winget install -e --id LLVM.LLVM --version <x.y.z>` si le composant VS ne suffit pas), OpenCppCoverage (`winget install -e --id OpenCppCoverage.OpenCppCoverage`).
- `git init` côté configuration locale (identité vérifiée), `.gitignore` complété, hook de pré-commit, presets, workflow CI, scripts, README.
- `CMakeUserPresets.json` local (chemins SDK) ; **aucune modification du PATH** (les scripts utilisent des chemins absolus du CMake de VS).

---
