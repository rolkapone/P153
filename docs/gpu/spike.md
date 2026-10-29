# Spike de faisabilité GPU — phase 1 (état machine, téléchargements, options)

Statut : phase 1 du 2026-10-21 (gpu-engineer). Stories : ENH-001 (super-résolution), ENH-002 (interpolation).
Phase 1 = inventaire **en lecture seule** de la machine du PO + plan. Rien n'a été installé ni téléchargé.
Toute valeur marquée **[H]** est une hypothèse / un ordre de grandeur à mesurer en phase 2. Les numéros de version des SDK NVIDIA sont à **relever sur la page de téléchargement** (ne pas se fier à ce document) puis à recopier ici.

## 1. État de la machine (relevé du 2026-10-21)

| Élément | Constaté | Verdict |
|---|---|---|
| GPU | NVIDIA GeForce RTX 5070, compute capability 12.0 (`sm_120`), PCIe gen 5, pilote en mode WDDM | OK |
| VRAM | 12 227 Mio au total, **2 052 Mio déjà utilisés** au repos (bureau Windows, Edge, iCUE, Docker Desktop, etc.) | Attention : ~10,1 Gio libres réellement ; budget job de 9 Gio = marge ~1 Gio |
| Pilote | 617.42, « CUDA UMD Version 13.4 » (le pilote accepte des runtimes CUDA jusqu'à 13.4) | OK (largement au-dessus de ce que demandent les SDK récents) |
| DLL du pilote | `nvcuda.dll`, `nvcuvid.dll` (NVDEC), `nvEncodeAPI64.dll` (NVENC), `nvofapi64.dll` (Optical Flow), `nvml.dll` dans `System32` ; NGX Core installé (registre `NGXCore`) | OK : les moteurs matériels sont exposés |
| CUDA Toolkit | **Absent** : pas de `nvcc`, pas de `CUDA_PATH`, pas de `C:\Program Files\NVIDIA GPU Computing Toolkit` | **Bloquant** pour compiler |
| Visual Studio | **Visual Studio Community 2026** (18.9), MSVC **14.51** (toolset v145), Windows SDK 10.0.26100, charges « Développement Desktop C++ » ; **pas de VS 2022** | Écart avec `CLAUDE.md` (« VS 2022 ») → ADR devops ; compatibilité CUDA à vérifier (§4) |
| CMake | Dans le PATH : `cmake 4.4.2` de **MSYS2 MinGW** (`C:\msys64\mingw64\bin`) ; intégré à VS : `cmake 4.3.1-msvc1` + Ninja | Utiliser celui de VS (ou un CMake officiel), pas celui de MSYS2 |
| clang-format / clang-tidy | Présents dans `VS\VC\Tools\Llvm\x64\bin` | OK |
| Git | 2.56.0.windows.2 | OK |
| FFmpeg / ffprobe | **Absents** (ni PATH, ni MSYS2) | À installer (§2.2) |
| Python | Seulement le Python 3.14 de MSYS2 MinGW (pas de `python.org`), **pas de PyTorch** | Inutilisable pour PyTorch ; nécessaire seulement pour l'export ONNX de RIFE et les scripts QA |
| vcpkg | Absent (ni `VCPKG_ROOT`, ni composant VS) | À installer (§2.2) |
| Qt | Absent | À installer (§2.2), concerne ENH-004 (developer) |
| `third_party/` | Vide | En attente des dépôts du PO |

## 2. Ce que le PO doit télécharger

### 2.1 SDK NVIDIA (compte NVIDIA Developer Program **obligatoire**, gratuit)

Tout se pose sous `F:\P153\third_party\nvidia\` (dossier ignoré par git). Garder l'archive **et** le fichier de licence du SDK : secdevops en a besoin (ENH-006). Noter le numéro de version exact dans le tableau ci-dessous une fois téléchargé.

| # | Produit | Page | Version minimale | Où le poser | Pour |
|---|---|---|---|---|---|
| 1 | **RTX Video SDK** (Windows) | https://developer.nvidia.com/rtx-video-sdk | dernière version publiée | `F:\P153\third_party\nvidia\rtx-video-sdk\` | ENH-001 (super-résolution), plus tard SDR→HDR10 |
| 2 | **Video Codec SDK** (NVENC/NVDEC, en-têtes + exemples) | https://developer.nvidia.com/video-codec-sdk | **13.0 ou plus** (prise en charge Blackwell) — prendre la dernière | `F:\P153\third_party\nvidia\video-codec-sdk\` | ENH-010/011, décodage, exemples `AppDec`/`AppEncCuda` |
| 3 | **Optical Flow SDK** (contient la bibliothèque FRUC) | https://developer.nvidia.com/optical-flow-sdk | **5.0 ou plus** — prendre la dernière | `F:\P153\third_party\nvidia\optical-flow-sdk\` | ENH-002 (candidat FRUC) |
| 4 | **TensorRT** pour Windows, archive ZIP (pas l'installeur pip) | https://developer.nvidia.com/tensorrt | **10.x** récent, variante qui correspond au CUDA Toolkit installé (CUDA 13.x) | `F:\P153\third_party\nvidia\tensorrt\` | ENH-002 (candidat RIFE) |

Version réellement téléchargée (relevée en phase 2) : RTX Video SDK `1.1.0` / Video Codec SDK `13.1.15` (API NVENC 13.1) / Optical Flow SDK `5.0.7` / TensorRT `11.4.0.106` Enterprise (CUDA 13.4) ; CUDA Toolkit `13.4` (V13.4.92) ; pilote `617.42`.

### 2.2 Installables **sans compte** (à installer par le PO, ou par devops avec son accord)

| # | Produit | Source | Version | Où | Remarque |
|---|---|---|---|---|---|
| 5 | **CUDA Toolkit** | https://developer.nvidia.com/cuda-downloads (pas de compte nécessaire) | **≥ 12.8 exigé** ; recommandé : dernière 13.x **dont les notes de version listent Visual Studio 2026 / MSVC 14.5x** comme compilateur pris en charge | Installation par défaut (`C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.x`) — **décocher « Driver »** pour garder le pilote 617.42 | Fournit `nvcc`, `cudart`, NVML, nvJPEG… |
| 6 | **FFmpeg, build LGPL partagé** (pour lier l'app) | https://github.com/BtbN/FFmpeg-Builds/releases, archive `ffmpeg-n8.x-latest-win64-lgpl-shared-8.x.zip` | branche 8.x épinglée | `F:\P153\third_party\ffmpeg\lgpl-shared\` | DLL + en-têtes + `.lib` ; NVENC/NVDEC inclus ; licence à valider par secdevops |
| 7 | **FFmpeg « outil » avec libvmaf** (QA uniquement, jamais redistribué) | https://www.gyan.dev/ffmpeg/builds/ (« full ») ou BtbN `gpl` | 8.x | hors repo, ex. `C:\Tools\ffmpeg\` + PATH | GPL acceptable car outil local de mesure, pas lié à Enhance |
| 8 | **vcpkg** | `git clone https://github.com/microsoft/vcpkg` (ou composant « vcpkg » de l'installeur VS 2026) | commit épinglé dans `vcpkg-configuration.json` (ADR devops) | ex. `C:\Tools\vcpkg` + `VCPKG_ROOT` | ENH-003 |
| 9 | **Python 3.12 64 bits officiel** + PyTorch roues **cu128** ou plus récentes | https://www.python.org/downloads/windows/ puis `pip install torch --index-url https://download.pytorch.org/whl/cu128` | 3.12 | hors repo, venv dédié | Seulement pour exporter RIFE en ONNX et les scripts QA ; le Python MSYS2 ne convient pas |
| 10 | **ONNX Runtime** GPU (option) | https://github.com/microsoft/onnxruntime/releases | dernière, build CUDA 12/13 compatible | `F:\P153\third_party\onnxruntime\` | Alternative à TensorRT natif ; MIT |
| 11 | **Qt 6** (ENH-004, developer) | Qt Online Installer (**compte Qt requis**) ou vcpkg (`qtbase`, `qtdeclarative`, sans compte, compilation longue) | 6.8 LTS ou plus | à trancher par ADR | Binaires « MSVC 2022 64-bit » compatibles avec VS 2026 (même ABI v14x) |

**Ne pas télécharger les poids RIFE** tant que secdevops n'a pas statué sur leur licence (§3).

## 3. Options d'interpolation (ENH-002)

Rappel : le RTX Video SDK fait la super-résolution et le SDR→HDR, **pas** la génération d'images (à confirmer à la lecture de sa doc en phase 2). Il faut une brique séparée.

| Option | Qualité attendue | Vitesse attendue **[H]** (RTX 5070) | VRAM **[H]** | Licence / redistribution | Effort C++ |
|---|---|---|---|---|---|
| **A. RIFE 4.x** (Practical-RIFE) exporté en ONNX → **TensorRT** FP16 natif | La meilleure des options listées ; défauts connus sur textes fins, grands mouvements, coupures de plan | 720p : plusieurs centaines d'images générées/s ; 4K : quelques dizaines/s (≈ 9× plus de pixels) | 720p : < 1 Gio ; 4K : 2–4 Gio | Code MIT ; **poids : licence non explicite → à valider par secdevops** (bloquant si refus) ; DLL TensorRT redistribuables selon la licence TensorRT (à vérifier) | Moyen : export ONNX (Python, une fois), construction du moteur TensorRT **hors ligne ou au premier lancement** (spécifique à `sm_120` et à la version TensorRT), entrées/sorties CUDA sans copie CPU |
| **B. RIFE via ONNX Runtime + fournisseur TensorRT** | Identique à A | Légèrement inférieure à A | Un peu plus que A | ONNX Runtime MIT ; mêmes questions sur les poids et TensorRT | Plus faible que A (API plus simple), mais une dépendance de plus et moins de contrôle sur la mémoire |
| **C. NVIDIA Optical Flow SDK – FRUC** (moteur de flux optique matériel + bibliothèque `NvOFFRUC`) | Inférieure à RIFE (flux par blocs, halos, occlusions) ; correcte sur mouvements simples | Très rapide, y compris en 4K (moteur matériel dédié) | Faible (< 1 Gio) | Licence du SDK NVIDIA (redistribution de la DLL FRUC à vérifier) | Faible à moyen ; **risque : prise en charge Blackwell de la DLL FRUC à confirmer** (le moteur OF lui-même est présent : `nvofapi64.dll`) |
| D. Autres réseaux (IFRNet, FILM, GMFSS) | FILM/GMFSS ≥ RIFE sur grands mouvements | Nettement plus lents que RIFE | Plus élevée | Variables (à vérifier) | Élevé ; à garder en réserve pour « Qualité max » si RIFE déçoit |
| E. Référence : FFmpeg `minterpolate` / doublement d'images | Faible / nulle | CPU, lent | — | LGPL | Nul ; sert uniquement de point bas de comparaison VMAF |

DLSS Frame Generation (Streamline) est écarté : il exige des vecteurs de mouvement et une profondeur issus d'un moteur de jeu.

**Ordre d'évaluation recommandé** : 1) C (FRUC) — rapide à brancher, donne un plancher de vitesse et vérifie la prise en charge Blackwell ; 2) A (RIFE + TensorRT) — candidat qualité principal, à lancer dès que secdevops a statué sur les poids ; 3) B seulement si l'intégration TensorRT native coince ; D hors spike sauf échec qualité.

**Place dans la chaîne** : recommandation **interpoler avant la super-résolution, à 720p** : ≈ 9× moins de pixels, donc l'interpolation devient quasi gratuite, et la super-résolution traite 60 images/s au lieu de 30 (c'est elle qui coûte). Contre-partie à mesurer : la super-résolution agrandit aussi les défauts des images générées. Le spike mesure **les deux ordres** (720p→interp→SR et 720p→SR→interp 4K) et l'ADR tranche sur VMAF + images/s.

## 4. Risques et bloquants

| # | Risque / bloquant | Effet | Parade |
|---|---|---|---|
| B1 | CUDA Toolkit absent | Aucune compilation GPU | Installer CUDA (§2.2 n° 5) |
| B2 | SDK NVIDIA non déposés | Phase 2 impossible | Téléchargements §2.1 par le PO |
| B3 | **VS 2026 / MSVC 14.51 au lieu de VS 2022** | `nvcc` refuse un compilateur hôte non listé (« unsupported Microsoft Visual Studio version ») | Choisir une version de CUDA qui liste VS 2026 ; sinon ajouter dans l'installeur VS 2026 le composant « MSVC v143 – Outils de build VS 2022 » et le sélectionner (`-T v143`) ; `-allow-unsupported-compiler` seulement en dépannage. ADR devops (ENH-003) + mise à jour de `CLAUDE.md` par l'orchestrateur |
| R1 | Licence des poids RIFE non explicite | Option A/B inutilisable en production | Avis secdevops avant tout téléchargement ; repli C (FRUC) |
| R2 | FRUC non pris en charge sur Blackwell | Option C tombe | Vérifier la matrice du SDK + test en phase 2 |
| R3 | 2 Gio de VRAM déjà pris par le bureau | Budget 9 Gio trop juste | Mesurer le pic **en différence** par rapport au repos ; vérifier la marge réelle ; garder le budget [H] |
| R4 | Mesure de VRAM par processus impossible via NVML/`nvidia-smi` en mode WDDM (affiché `N/A`) | Mesures faussées | Mesurer dans le processus : `cuMemGetInfo` avant/après + `IDXGIAdapter3::QueryVideoMemoryInfo` (budget local du processus) ; en complément `nvidia-smi` global échantillonné |
| R5 | Moteur TensorRT lié à la carte et à la version TensorRT | Pas embarquable tel quel dans l'installeur | Construire le moteur au premier lancement, **en local, sans réseau**, et le mettre en cache ; ou livrer l'ONNX + cache |
| R6 | Licence RTX Video SDK / Video Codec SDK (redistribution des DLL) | Publication | Registre ENH-006 (secdevops) |
| R7 | FFmpeg GPL par erreur (builds « full » de gyan) liés à l'app | Licence de l'app contaminée | App liée **uniquement** au build BtbN `lgpl-shared` ; build GPL réservé aux outils QA |

## 5. Protocole de la phase 2 (après dépôt des SDK)

1. **Contrôle des dépôts** : lister `third_party\nvidia\*`, relever versions et fichiers de licence, transmettre à secdevops. Lire les en-têtes et la doc fournis (citer les fichiers dans `docs/gpu/spike-vsr.md`).
2. **Compiler les exemples officiels**, sans modification, avec CUDA + MSVC :
   - Video Codec SDK : `AppDec` (NVDEC → surfaces CUDA) et `AppEncCuda` (NVENC HEVC/AV1 depuis la mémoire CUDA) → valide la chaîne d'outils et donne le débit brut NVDEC 720p et NVENC 4K ;
   - RTX Video SDK : l'exemple fourni (super-résolution) sur l'interop CUDA si elle existe, sinon D3D11/D3D12 ;
   - Optical Flow SDK : l'exemple FRUC ;
   - TensorRT : `trtexec` sur l'ONNX RIFE (si poids validés), FP16, profils 1280×720 et 3840×2160.
3. **Clip** : `REF-A` défini par ENH-005 (720p 30 i/s issu d'un 4K réduit en Lanczos). En attendant : un clip libre 4K 60 i/s (licence CC à consigner dans `docs/qa/clips.md`), réduit en 720p30 par FFmpeg (`scale=1280:720:flags=lanczos`, `fps=30`), 20 s, plus sa version 720p60 pour juger l'interpolation (on retire une image sur deux, on régénère, VMAF sur les seules images générées vs l'original).
4. **Petit programme de mesure** `tests/gpu/spike_bench` (C++20, hors app) : NVDEC → super-résolution → (interpolation) → NVENC HEVC Main10, tout en surfaces CUDA, avec :
   - **images/s par étage** : événements CUDA (`cuEventRecord`/`cuEventElapsedTime`) autour de chaque étage, 30 images de chauffe ignorées, médiane et 5e centile sur le reste ;
   - **images/s de bout en bout** : horloge murale (`std::chrono::steady_clock`) du premier paquet lu au dernier paquet écrit ;
   - **pic de VRAM** : `cuMemGetInfo` au repos puis à chaque étape + `QueryVideoMemoryInfo` (DXGI) toutes les 100 ms dans un fil dédié ; en parallèle `nvidia-smi --query-gpu=memory.used --format=csv -lms 100` ; résultat = pic − repos, en Mio ;
   - **fuite** : 10 exécutions à la suite dans le même processus, dérive mémoire relevée.
5. **Matrice mesurée** : modes de qualité du RTX Video SDK × {interp avant SR, interp après SR} × {FRUC, RIFE}, encodage HEVC 10 bits et AV1 10 bits. Qualité : VMAF (modèle 4K), SSIM, PSNR vs 4K d'origine et vs Lanczos, selon `docs/qa/protocole.md`.
6. **Livrables** : `docs/gpu/spike-vsr.md` (ENH-001), `docs/gpu/spike-interpolation.md` (ENH-002), ADR de go / no-go et budgets i/s et VRAM dans `docs/architecture/index.md`.

---

# Phase 2 : résultats (2026-10-21, gpu-engineer)

Mesures sur la machine du PO (RTX 5070 12 Gio, pilote 617.42, WDDM, Windows 11 26200). **Entrées synthétiques générées sur le GPU** (mire animée : dégradés, disques en mouvement, bandeau « texte » binaire défilant, grain temporel réglable) : aucun clip réel n'était disponible. Vitesses et VRAM sont représentatives ; **la qualité (VMAF/SSIM/PSNR) n'est pas mesurée** et reste à faire par la QA sur `REF-A`. Tout est construit dans `build/spike/` (ignoré par git) ; aucun SDK modifié, aucun téléchargement.


## Sommaire (découpage du 2026-10-29)

- § 1-5 état de la machine, téléchargements, options, risques, protocole phase 2 : [spike.md](spike.md)
- Phase 2 : P2.1-P2.6 : [spike-phase2.md](spike-phase2.md)
- Phase 3 : P3.1-P3.8 : [spike-phase3.md](spike-phase3.md)
