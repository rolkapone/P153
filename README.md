# Enhance

Application Windows d'upscaling video « baked » (traitement de fichiers hors ligne), 100 % locale,
qui utilise l'API NVIDIA en natif sur GeForce RTX 50. Voir `docs/vision.md`.

## Prerequis (machine de developpement)

| Element | Version | Remarque |
|---|---|---|
| Windows | 11 | |
| Pilote NVIDIA | >= 617.42 | GeForce RTX 50 (Blackwell, `sm_120`) |
| Visual Studio | 2026 (18.x), charge « Developpement Desktop C++ » | MSVC 14.5x (toolset v145) ; CMake et Ninja de VS sont utilises par les scripts |
| CUDA Toolkit | 13.4 (>= 12.8 exige pour `sm_120`) | Version epinglee dans `scripts/gpu-requirements.json` |
| SDK NVIDIA | RTX Video SDK, Video Codec SDK, Optical Flow SDK, TensorRT | Telecharges par vous (compte NVIDIA), poses dans `third_party/nvidia/<sdk>/` ; jamais commites. Autre emplacement : variable `ENH_NVIDIA_SDK_DIR` |
| FFmpeg LGPL (shared) | BtbN n8.1.3, tag et SHA-256 dans `deps/ffmpeg.lock.json` | `scripts\fetch-ffmpeg.ps1` (reseau, une fois) le pose dans `third_party/ffmpeg/lgpl-shared/` ; `scripts\check-ffmpeg-license.ps1` refuse un build GPL/nonfree |
| vcpkg | commit epingle dans `vcpkg.json` (`builtin-baseline`) | clone dans `C:\Tools\vcpkg` par `scripts\bootstrap.ps1` (reseau, une seule fois) ; Qt 6.11.2 compile sans Qt6Network via les overlay ports de `vcpkg/overlay-ports/` |
| clang-format | 22.1.3 | composant LLVM de Visual Studio 2026 (identique a la CI) |
| gitleaks (optionnel) | 8.30.1 | `C:\Tools\gitleaks\gitleaks.exe`, utilise par le hook pre-commit s'il est present |
| OpenCppCoverage (optionnel) | 0.9.9.0 | `winget install OpenCppCoverage.OpenCppCoverage` ; `scripts\test.ps1 -Coverage` |

La CI GitHub n'a pas de carte RTX : elle compile et teste le monde « sans GPU » (pipeline simule).
Les tests GPU tournent en local.

## Commandes (PowerShell, depuis la racine du depot)

```powershell
scripts\install-hooks.ps1                 # une fois : hooks pre-commit et commit-msg + identite git locale
scripts\bootstrap.ps1 -Run -Install       # une fois : vcpkg + dependances (compile Qt, ~30 min a 1 h 20)
scripts\fetch-ffmpeg.ps1                  # une fois : FFmpeg LGPL de developpement (verifie le SHA-256)
scripts\build.ps1 -Preset dev             # configure + compile (GPU et UI si disponibles)
scripts\build.ps1 -Preset ci              # comme la CI : sans GPU, sans UI, /WX
scripts\test.ps1                          # tests unitaires (sans GPU)
scripts\test-gpu.ps1                      # tests GPU + rapport dans docs\qa\resultats\
scripts\test-gpu.ps1 -CheckOnly           # verifie seulement les prerequis (code 0/1/2)
scripts\format.ps1 [-Check]               # clang-format
scripts\check-no-network.ps1 [-SourcesOnly] [-Path build\ci]   # preuve zero reseau (sources, imports, chaines)
scripts\test.ps1 -Coverage                # + couverture (OpenCppCoverage)
```

Presets CMake : `dev`, `dev-nogpu`, `local-gpu` (GPU obligatoire), `release`, `ci`, `ci-ui`.
Architecture et decisions : `docs/architecture.md`.

## Conventions

Commits Conventional Commits ; une story = une branche = un merge ; aucun appel reseau dans
l'application ; ne jamais commiter de SDK, de binaire tiers, de clip video ni de secret.
