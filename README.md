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
| FFmpeg LGPL (shared) | voir ADR-025 | `third_party/ffmpeg/lgpl-shared/` |
| vcpkg | commit epingle dans `vcpkg.json` | installe par `scripts\bootstrap.ps1` (necessite le reseau, une seule fois) |

La CI GitHub n'a pas de carte RTX : elle compile et teste le monde « sans GPU » (pipeline simule).
Les tests GPU tournent en local.

## Commandes (PowerShell, depuis la racine du depot)

```powershell
scripts\install-hooks.ps1                 # une fois : hook pre-commit + identite git locale
scripts\bootstrap.ps1 -Baseline <sha> -Run -Install   # une fois : vcpkg + dependances (phase B)
scripts\build.ps1 -Preset dev             # configure + compile (GPU et UI si disponibles)
scripts\build.ps1 -Preset ci              # comme la CI : sans GPU, sans UI, /WX
scripts\test.ps1                          # tests unitaires (sans GPU)
scripts\test-gpu.ps1                      # tests GPU + rapport dans docs\qa\resultats\
scripts\test-gpu.ps1 -CheckOnly           # verifie seulement les prerequis (code 0/1/2)
scripts\format.ps1 [-Check]               # clang-format
```

Presets CMake : `dev`, `dev-nogpu`, `local-gpu` (GPU obligatoire), `release`, `ci`, `ci-ui`.
Architecture et decisions : `docs/architecture.md`.

## Conventions

Commits Conventional Commits ; une story = une branche = un merge ; aucun appel reseau dans
l'application ; ne jamais commiter de SDK, de binaire tiers, de clip video ni de secret.
