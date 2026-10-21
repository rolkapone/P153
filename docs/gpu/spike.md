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
6. **Livrables** : `docs/gpu/spike-vsr.md` (ENH-001), `docs/gpu/spike-interpolation.md` (ENH-002), ADR de go / no-go et budgets i/s et VRAM dans `docs/architecture.md`.

---

# Phase 2 : résultats (2026-10-21, gpu-engineer)

Mesures sur la machine du PO (RTX 5070 12 Gio, pilote 617.42, WDDM, Windows 11 26200). **Entrées synthétiques générées sur le GPU** (mire animée : dégradés, disques en mouvement, bandeau « texte » binaire défilant, grain temporel réglable) : aucun clip réel n'était disponible. Vitesses et VRAM sont représentatives ; **la qualité (VMAF/SSIM/PSNR) n'est pas mesurée** et reste à faire par la QA sur `REF-A`. Tout est construit dans `build/spike/` (ignoré par git) ; aucun SDK modifié, aucun téléchargement.

## P2.1 Chaîne d'outils

| Élément | Résultat |
|---|---|
| `nvcc` 13.4 (V13.4.92) + MSVC 19.51 (v145, VS 2026 18.9) | **OK sans `-allow-unsupported-compiler`**, aucun avertissement ; programme trivial `-arch=sm_120` : cubin `sm_120`, noyau exécuté sur la 5070 (cc 12.0). B3 levé |
| CMake | utiliser celui de VS (`4.3.1-msvc1`) + Ninja ; celui de MSYS2 (4.4.2, premier dans le PATH) est à éviter |
| Video Codec SDK 13.1.15 : `AppEncCuda`, `AppEncPerf` (+ 7 autres `AppEnc*`) | **compilent et tournent** (`-DSKIP_FFMPEG_DEPENDENCY=1`). `AppDec*` non construits : ils exigent FFmpeg (absent) |
| Optical Flow SDK 5.0.7 : `NvOFFRUCSample` | **compile et tourne** sur la 5070 (NV12 720p synthétique, 60 images, 2,78 ms par image interpolée, 0 répétition) |
| RTX Video SDK 1.1.0 | exemples du `.sln` (MFC/ATL, Vulkan) non construits ; **interface CUDA validée par le banc** (`NGX_CUDA_CREATE_VSR` / `NGX_CUDA_EVALUATE_VSR`), après contournement du **bloquant B4** ci-dessous |
| TensorRT 11.4 | `trtexec.exe` démarre (`TensorRT v110400`), `nvinfer_builder_resource_sm120_11.dll` présent. Non mesuré : pas d'ONNX RIFE (poids non validés) |

### B4 — RTX Video SDK 1.1.0 incompatible avec le runtime CUDA 13 (résolu par isolement)

- Symptôme : le processus meurt dans `NVSDK_NGX_CUDA_Init` avec `0xC0000409` (STATUS_STACK_BUFFER_OVERRUN), sans code retour.
- Cause (vérifiée) : `lib/Windows/x64/nvsdk_ngx_{s,d}.lib` est compilée avec les en-têtes **CUDA 11.8** (chemins `cuda\11.8.0-065` dans la lib) et importe 3 fonctions du runtime CUDA : `cudaGetDevice`, `cudaGetDeviceProperties`, `cudaGetErrorString` (`dumpbin /symbols`). En CUDA 13.4, `struct cudaDeviceProp` a changé (`clockRate`, `deviceOverlap`, `computeMode`… retirés, `reserved[56]` ajouté : `include/driver_types.h` l. 2673) : le `cudart` 13 écrit une structure plus grande que le tampon 11.8 que NGX a sur sa pile.
- Contournement validé : **DLL pont `rtxvideo_bridge.dll`** qui contient `nvsdk_ngx_s.lib` **sans lier `cudart`** et fournit elle-même ces 3 fonctions via l'API driver (n'écrit que le préfixe 11.x de `cudaDeviceProp`). Le reste de l'application garde CUDA 13.4. C'est aussi le bon découpage pour la production (chargement différé, ADR-048). À relire par le reviewer, à signaler à NVIDIA, à revoir si une version plus récente du SDK sort.

## P2.2 Ce que disent les SDK (« à vérifier » levés)

| Question | Réponse | Source |
|---|---|---|
| RTX Video SDK : interface CUDA ? | **Oui** (DX11, DX12, CUDA, Vulkan sous Windows). CUDA : création avec `CUcontext` + `CUstream`, évaluation `CUtexObject` (entrée) → `CUsurfObject` (sortie) adossés à des `CUarray` ; variantes `CUdeviceptr`/hôte seulement via copie interne | `include/nvsdk_ngx_helpers_vsr.h` (`NVSDK_NGX_CUDA_VSR_Create_Params`, `NGX_CUDA_EVALUATE_VSR`), `samples/RTX_Video_API/rtx_video_api.h`, `rtx_video_api_cuda_impl.cpp` |
| Formats VSR | **Entrée RGB uniquement** : ARGB 8 bits (`CU_AD_FORMAT_UNSIGNED_INT8`×4) ou ABGR10 (`CU_AD_FORMAT_UNORM_INT_101010_2`) ; sortie ARGB8, ou ABGR10 si l'entrée est 10 bits. Pas de NV12/P010 (les démos DX convertissent par VpBlt) → **conversions YUV→RGB et RGB→P010 à notre charge** (noyaux CUDA, ADR-042) | guide `doc/NVIDIA_RTX_Video_SDK_Programming_Guide.pdf` §3.3 et §5.2 ; en-tête de `rtx_video_api_cuda_impl.cpp` |
| 10 bits | **Oui** (ABGR10 en entrée et en sortie) ; mesuré aussi rapide qu'en 8 bits | banc `vsr --bits 10` |
| Facteur d'échelle | **Libre** : rectangles d'entrée et de sortie arbitraires | `NVSDK_NGX_CUDA_VSR_Eval_Params` |
| Niveaux de qualité | `0` bicubique, `1` Low, `2` Medium, `3` High, `4` Ultra ; paramètre d'évaluation, changeable à chaque image | `include/nvsdk_ngx_defs_vsr.h` |
| Mode « hors ligne » | Pas de mode dédié : appel par image, sans contrainte temps réel ; convient au traitement de fichiers | guide §5.2 |
| SDR → HDR | **TrueHDR** (même SDK) : entrée RGB SDR, sortie ABGR10 ou FP16 ; Contrast/Saturation/MiddleGray (défaut 100/100/50), MaxLuminance 400–2000 nits ; CUDA si pilote ≥ r570 | `nvsdk_ngx_helpers_truehdr.h`, guide §5.1 |
| Pilote minimal | r550.58 (VSR ; NGX rapporte `VSR.MinDriverVersion` = 550.0), r570 pour TrueHDR en CUDA ; CUDA Toolkit ≥ 12.8 | guide §2.1 ; banc |
| Redistribution | `nvngx_vsr.dll` (19 Mo) et `nvngx_truehdr.dll` du dossier `bin/Windows/x64/rel` (sans filigrane), à côté de l'exe ; App ID `0` tant que NVIDIA n'en attribue pas | guide ch. 4 |
| **Réseau** | Le guide décrit un **« NGX Update Module »** (composant du pilote) qui peut **télécharger une version plus récente de la DLL de la fonction** à l'initialisation. **À instruire par secdevops** (règle « zéro réseau ») : c'est le pilote, pas notre code, mais il faut savoir si on peut l'empêcher | guide ch. 1 |
| FRUC : Blackwell | **Fonctionne sur la 5070**, mais `NvOFFRUC.dll` ne contient que du code `sm_75`/`sm_80` + PTX (`cuobjdump --list-elf/--list-ptx`) : compilé à la volée par le pilote au premier lancement (~1,3 s, puis cache), non optimisé Blackwell. Dépend de `cudart64_110.dll` (CUDA 11.0), à redistribuer | banc ; `NvOFFRUC/NvOFFRUCSample/bin/win64` |
| FRUC : formats | **8 bits seulement** : NV12 ou ARGB ; ressources `CUdeviceptr` ou `CUarray` (≤ 10 enregistrées) ; **instant interpolé arbitraire** (0,25 / 0,5…) ; drapeau « image répétée » quand il juge l'interpolation trop mauvaise | `NvOFFRUC/Interface/NvOFFRUC.h`, `Doc/NVOFA_FRUC.pdf` §3.4 |
| NVENC sur la 5070 | **1 moteur NVENC** (H.264, HEVC, AV1) ; AV1 10 bits oui (4:2:0 seulement) ; anticipation ≤ 31 − B-frames ; réglage `UHQ` disponible en HEVC et AV1 | `AppEncCuda -ec` ; `Interface/nvEncodeAPI.h` l. 2225 |
| NVENC API 13.1 | `NV_ENC_PIC_FLAG_FORCEIDR` (l. 680), `nvEncSetIOCudaStreams` (l. 4645), HDR10 : `MASTERING_DISPLAY_INFO` / `CONTENT_LIGHT_LEVEL` (l. 1711-1728), `outputMasteringDisplay` / `outputMaxCll` en HEVC (l. 1972-1973) **et en AV1** (l. 2095-2096), pointeurs `pMasteringDisplay` / `pMaxCll` par image (l. 2493, 2552) | `Interface/nvEncodeAPI.h` |

## P2.3 Mesures

Banc `build/spike/bench/` (`spike_bench.exe`, C++20 + CUDA sm_120), commandes `vsr`, `thdr`, `nvenc`, `fruc`, `chain`. Méthode : 30 images de chauffe ignorées (60 pour `chain`) ; débit = `steady_clock` sur le reste, après synchronisation ; temps par appel = événements CUDA (1 image sur 10) ; **VRAM = pic de `IDXGIAdapter3::QueryVideoMemoryInfo` (usage local du processus) échantillonné toutes les 5 ms, moins la valeur juste avant la création de l'étage** (`cuMemGetInfo` donne le même delta). Le contexte CUDA seul coûte ~140 Mio de VRAM de processus.

### VSR 1280×720 → 3840×2160 (RTX Video SDK, CUDA, DLL `rel`)

`spike_bench vsr --q <0..4> --bits <8|10> --frames 300`

| Qualité | 8 bits i/s | ms/image | 10 bits i/s | VRAM de l'effet (Mio) |
|---|---|---|---|---|
| 0 bicubique | 2 220 | 0,44 | 3 066 | 76 |
| 1 Low | 972 | 1,00 | 1 096 | 76 |
| 2 Medium | 822 | 1,19 | 917 | 82 |
| 3 High | 421 | 2,32 | 445 | 188 |
| 4 Ultra | **322** | 3,04 | **335** | 188 |

Tampons d'E/S en plus (4 entrées 720p + 1 sortie 4K) : 48 Mio. Création de l'effet : ~0,8 s (10,7 s au tout premier lancement sur la machine, ensuite en cache). Contrôle : les sorties `q=0` et `q=4` diffèrent sur 59 % des octets (le réseau tourne bien). **La super-résolution, « inconnue principale » de l'architecture, est ~3-5× plus rapide que l'hypothèse (70–120 i/s) et ~14× moins gourmande en VRAM (188 Mio contre 2 560 prévus).**

TrueHDR 4K → 4K (pour ENH-032) : **737 i/s, 164 Mio**.

### NVENC 3840×2160, 10 bits (P010 déjà en VRAM, copie D2D comprise)

`spike_bench nvenc --frames 600 --bits 10 --opts "<options>"` (syntaxe de `Samples/Utils/NvEncoderCLIOptions.h`)

| Préréglage proposé | Options | HEVC i/s | HEVC VRAM (Mio) | AV1 i/s | AV1 VRAM (Mio) |
|---|---|---|---|---|---|
| Rapide | `-preset p3 -tuninginfo hq -rc vbr`, 25 / 19 Mbit/s | 265 | 362 | 178 | 626 |
| Équilibré | `-preset p5 -tuninginfo hq -rc vbr -multipass qres -lookahead 16`, 40 / 30 Mbit/s | **103** | 869 | **105** | 1 266 |
| Qualité max (HQ) | `-preset p7 -tuninginfo hq -multipass fullres -lookahead 28 -bf 3`, 60 / 45 Mbit/s | **37** | 1 265 | **52** | 1 458 |
| Qualité max (UHQ) | `-preset p7 -tuninginfo uhq`, 60 / 45 Mbit/s | 29 | 2 094 | 38 | 2 262 |

Repères : HEVC P5 HQ **sans** multipasse ni anticipation = 119 i/s ; HEVC Équilibré en 8 bits = 106 i/s / 493 Mio. Débits obtenus : à ±12 % de la cible.

### FRUC (Optical Flow SDK), ARGB 8 bits, temps par image interpolée (synchronisation comprise)

`spike_bench fruc --size <WxH> --res <array|devptr> --frames 300 --noise <a>`

| Résolution | Grain 0 | Grain ±0,5 % | Grain ±1 % | Grain ≥ ±2 % | VRAM session (Mio) |
|---|---|---|---|---|---|
| 1280×720 | 2,7 ms (≈ 365 i/s) | 3,2 ms | 3,8 ms | **100 % d'images répétées** | 98 |
| 1920×1080 | 5,9 ms (168 i/s) | | | | 219 |
| 3840×2160 | 22,7 ms (44 i/s) | | | | 735 |

**Constat qualité important** : dès un grain de ±2 % (≈ ±5 niveaux sur 255, courant sur du film ou un téléphone en basse lumière), FRUC renonce et **répète l'image précédente pour toutes les images** (drapeau `bHasFrameRepetitionOccurred`) : la sortie serait un 30 i/s doublé, pas un vrai 60 i/s. Même chose en chaîne « interpolation après SR » à 4K, sans grain (299/299 répétitions sur la sortie VSR accentuée). À confirmer sur clips réels, mais c'est un risque fort pour l'option C.

### Chaîne complète 720p30 → 4K60 (sans décodage : source générée sur le GPU)

`spike_bench chain --frames 300 --noise 0 --order <before|after|none> --q <3|4> --opts "<Équilibré>"` : thread de traitement (FRUC → VSR → RGB→P010) + thread NVENC, file bornée de 4 tampons 4K P010.

| Ordre | VSR | Codec (Équilibré) | Sortie 4K i/s | VRAM pic, delta (Mio) |
|---|---|---|---|---|
| FRUC 720p **avant** SR | 3 High | HEVC | **107** | 1 293 |
| FRUC 720p avant SR | 4 Ultra | HEVC | **104** | 1 293 |
| FRUC 720p avant SR | 3 High | AV1 | **118** | 1 690 |
| FRUC 4K **après** SR | 3 High | HEVC | 92 (100 % d'images répétées) | 1 984 |
| sans interpolation (30 → 30) | 3 High | HEVC | 101 | 1 197 |

Dans tous les cas **NVENC est le goulot** (≈ 103–118 i/s, cohérent avec l'étage seul) ; les SM ne sont pas saturés.

**Fuite** : 10 jobs à la suite dans le même processus (`--repeat 10`, contexte gardé) : VSR + NVENC **0 Mio** de dérive ; avec FRUC **+2 Mio par job** (création/destruction de session `NvOFFRUC`), soit +20 Mio sur 10 jobs. Parade si FRUC est retenu : une session réutilisée par résolution.

## P2.4 Estimation 720p30 → 4K60 et verdict

Temps SM par image de sortie, interpolation avant SR (½ image générée par image de sortie) : VSR 2,3 ms (q3) ou 3,0 ms (q4) + ½ × FRUC 2,7 ms + conversions ~0,2 ms ≈ **3,9–4,6 ms, soit ~220–260 i/s côté SM** : le débit est plafonné par NVENC.

| Préréglage | Plafond NVENC HEVC / AV1 | Chaîne (mesurée ou estimée, + NVDEC et démux/mux non mesurés) | Cible | Verdict |
|---|---|---|---|---|
| Rapide (VSR q2, P3) | 265 / 178 | ≈ 170–200 (SM ≈ 2,6 ms/image) | ≥ 100 | **OK** |
| Équilibré (VSR q3–4, P5 + multipasse ¼ + anticipation 16) | 103 / 105 | **mesuré 104–118** | ≥ 60 | **OK, marge ~1,7×** |
| Qualité max (VSR q4, P7 HQ multipasse pleine) | 37 / 52 | ≈ 35 / 50 | ≥ 30 | **OK de justesse** (UHQ HEVC 29 i/s : sous la cible) |

VRAM : pic mesuré de la chaîne Équilibré HEVC ≈ **1,3 Gio** (+ contexte 0,14 Gio), contre **7 656 Mio prévus** et 9 216 de budget. Même en ajoutant RIFE TensorRT (~1 Gio), NVDEC (0,6 Gio) et le pire encodeur (UHQ, 2,3 Gio), on reste **sous ~4,5 Gio**.

**Verdict : 720p30 → 4K60 est faisable sur la RTX 5070, avec de larges marges en vitesse et en VRAM.** Le facteur limitant est l'unique NVENC, pas la super-résolution. Le vrai risque restant est **la qualité de l'interpolation** : FRUC est rapide mais abandonne (répétition d'images) sur du contenu granuleux ; RIFE reste le candidat qualité (option A), à mesurer dès que les poids sont validés. La marge SM laisse la place à RIFE : à 720p, même à 10 ms par image générée, l'Équilibré resterait limité par NVENC.

## P2.5 Commandes exactes

```bat
:: environnement : vcvars64 (VS 2026) + CUDA 13.4 + CMake/Ninja de VS (script build\spike\scripts\vcenv.bat)
vcenv.bat nvcc -std=c++20 -arch=sm_120 -O2 -o hello.exe hello.cu
vcenv.bat cmake -S third_party\nvidia\video-codec-sdk\Samples -B build\spike\vcsdk -G Ninja -DCMAKE_BUILD_TYPE=Release -DSKIP_FFMPEG_DEPENDENCY=1 -DSKIP_GL_DEPENDENCY=1 -DAUTO_LOCATE_VULKAN=OFF
vcenv.bat cmake --build build\spike\vcsdk --target AppEncCuda AppEncPerf
vcenv.bat cmake -S third_party\nvidia\optical-flow-sdk\NvOFFRUC\NvOFFRUCSample -B build\spike\fruc_sample\build -G Ninja -DCMAKE_BUILD_TYPE=Release
vcenv.bat cmake -S build\spike\bench -B build\spike\bench\out -G Ninja -DCMAKE_BUILD_TYPE=Release
vcenv.bat cmake --build build\spike\bench\out
:: mesures (dossier build\spike\bench\out) ; sorties brutes dans build\spike\results\*.txt
spike_bench vsr --q 4 --bits 10 --frames 300
spike_bench thdr --size 3840x2160 --frames 200
spike_bench nvenc --frames 600 --bits 10 --opts "-codec hevc -profile main10 -preset p5 -tuninginfo hq -rc vbr -bitrate 40M -fps 60 -multipass qres -lookahead 16"
spike_bench fruc --size 1280x720 --res array --frames 300 --noise 0
spike_bench chain --frames 300 --noise 0 --order before --q 3 --opts "<options Équilibré>"
spike_bench chain --frames 60 --warm 10 --noise 0 --order before --q 3 --repeat 10 --opts "<options Équilibré>"
NvOFFRUCSample.exe "--input=F:/P153/build/spike/fruc_run/synth_1280x720.yuv" --width=1280 --height=720 "--output=F:/P153/build/spike/fruc_run/out" --surfaceformat=0 --allocationtype=0 --cudasurfacetype=0
```

Le code du banc (`build/spike/bench/` : `main.cpp`, `synth.cu`, `rtxvideo_bridge.cpp/.h`, `CMakeLists.txt`) est jetable ; s'il doit être conservé, le déplacer dans `tests/gpu/spike_bench/` sur une branche dédiée.

## P2.6 Limites et suites

- NVDEC et FFmpeg non mesurés (FFmpeg absent) : décodage 720p supposé négligeable (moteur séparé), à mesurer dès que le build LGPL est posé.
- Qualité non mesurée (pas de clip) : VMAF de VSR q3/q4 vs Lanczos, FRUC vs RIFE, ordre avant/après → QA sur `REF-A`.
- RIFE non mesuré (poids non validés, pas d'export ONNX).
- Aucun fichier de licence TensorRT dans l'archive dépliée (seulement `doc/README.txt`, `doc/Acknowledgements.txt`) : à récupérer pour secdevops.
- Pour secdevops : NGX Update Module (téléchargement de DLL par le pilote), redistribution de `nvngx_vsr.dll`, de `NvOFFRUC.dll` + `cudart64_110.dll`, des DLL TensorRT.

---

# Phase 3 : RIFE (2026-10-21, gpu-engineer)

**Essai local autorisé par le PO le 2026-10-21, sans livraison** : les poids RIFE n'ont pas encore de licence validée (finding SEC-03). Modèle, moteurs TensorRT et caches sont **uniquement** dans `third_party/models/rife/` et `build/spike/` (ignorés par git, vérifié par `git check-ignore`). TensorRT 11.4 utilisé en local (licence en cours d'analyse par secdevops).

## P3.1 Modèle utilisé

| Élément | Valeur |
|---|---|
| Modèle | **RIFE 4.25** (Practical-RIFE ; le README d'origine la recommande « par défaut pour la plupart des scènes »), export ONNX « v2 » de vs-mlrt (gère le bourrage en interne) |
| Source | `https://github.com/AmusementClub/vs-mlrt/releases/download/external-models/rife_v4.25.7z` (release `external-models` du dépôt AmusementClub/vs-mlrt, fichier publié le 2024-09-19 par WolframRhodium) |
| Licence affichée | dépôt **vs-mlrt : GPL-3.0** (licence du dépôt GitHub ; la release ne précise pas de licence propre aux fichiers ONNX). Poids d'origine : dépôt `hzwer/Practical-RIFE` sous **MIT** (commit `bbfd2ea9`, 2026-08-27), README, section « Trained Model » : *« The content of these links is under the same MIT license as this project. »* → **à instruire par secdevops (SEC-03)** : les poids d'origine sont annoncés MIT, mais l'ONNX converti vient d'un dépôt GPL-3.0 |
| SHA-256 archive `rife_v4.25.7z` (21 026 388 octets) | `172fe975c1775134bb87108e4ec6d1a89e861cc5d3be2ac23bf08afe5ed626b8` |
| SHA-256 `rife/rife_v4.25.onnx` (v1, entrée 11 canaux, non utilisé) | `084f56a0c1e49f404d7a0a171b62a5d49078efdb02fea14d969069e604c60fe9` |
| SHA-256 `rife_v2/rife_v4.25.onnx` (**utilisé**, 22 748 049 octets) | `65c57a5e4abb17ad67faf35054291ac53affab0506d509d2e66698f6ecd75584` |
| Entrée / sortie (lues par le parseur TensorRT) | `input` FP32 `[N, 7, H, W]` = image 0 RGB, image 1 RGB, plan « instant t » ; `output` FP32 `[1, 3, H, W]` ; RGB dans [0, 1] |
| Emplacement | `F:\P153\third_party\models\rife\` (`_archives\`, `rife\`, `rife_v2\`, `fp16\` = dérivés) |

Aucun Python n'a été nécessaire : l'ONNX était déjà exporté.

## P3.2 TensorRT 11 n'a plus de `--fp16` : conversion de précision maison

TensorRT 11.4 ne construit que des réseaux **« strongly typed »** : la précision vient des types de l'ONNX (`trtexec --help` : `--stronglyTyped [Deprecated] Strongly typed network is now enabled by default` ; plus d'option `--fp16` ; `include/NvInfer.h` l. 11552 : `kSTRONGLY_TYPED TRT_DEPRECATED_ENUM`). Un ONNX FP32 donne un moteur FP32 (+ TF32). Pour obtenir du FP16 sans Python : petit outil C++ `build/spike/rife/tools/onnx_fp16.cpp` qui réécrit directement le protobuf ONNX (format filaire, champs de `onnx.proto3`).

| Variante | Conversion | SHA-256 de l'ONNX dérivé |
|---|---|---|
| **FP16 complet** (`fp16/rife_v4.25_v2_fp16.onnx`) | tous les tenseurs FLOAT → FLOAT16 (sauf `scales` des `Resize`), E/S en FP16 | `f994644767df43fe41d4b99c1c91d10c20c2462aec98c7ecc17d97834eb4a3f7` |
| **Mixte, convolutions FP16** (`fp16/rife_v4.25_v2_convfp16.onnx`) | les 59 `Conv`/`ConvTranspose` en FP16, entourées de `Cast` ; grilles de déformation, `GridSample`, `Resize`, additions de flux restent en FP32 ; E/S FP32 | `dea262e271d98ebc28a6336f13007812c3016878de6f79b2fed1967140cb3095` |
| FP32 (+TF32) | ONNX d'origine | — |

**Constat : le FP16 complet dégrade nettement la qualité** (P3.5 : −4 à −8 points VMAF) : les coordonnées de la grille d'échantillonnage (`Range` → `Cast` → normalisation dans [−1, 1]) perdent leur précision sous-pixel en FP16. **La variante mixte a la qualité du FP32** (PSNR entre les deux sorties : 67,5 dB ; VMAF identique à 0,04 près) pour 70 % de son temps de calcul. → **Variante retenue : mixte.** En production, cette conversion se fait une fois pour toutes à la préparation du modèle (outil de build), jamais dans l'application.

## P3.3 Construction des moteurs (TensorRT 11.4.0.106, `sm_120`, pilote 617.42)

`trtexec --onnx=<onnx> --shapes=input:1x7xHxW --saveEngine=<moteur> --timingCacheFile=<cache>` ; temps « Engine built in » de trtexec (analyse ONNX + optimisation + sérialisation). **CPU chargé à 60–100 % pendant ces mesures** (compilation vcpkg de Qt en parallèle) : les temps de construction sont pessimistes.

| Moteur | Résolution | Construction (1er lancement) | Avec cache de timing | Taille du moteur | Taille du cache de timing | Mémoire de travail du contexte |
|---|---|---|---|---|---|---|
| FP32/TF32 | 720p | 21 s | — | 29,3 Mo | — | 291 Mio |
| FP16 complet | 720p | 65 s | **9,8 s** | 26,1 Mo | 6,0 Mo | 146 Mio |
| **Mixte** | **720p** | **144 s (84,5 s à vide, P3.7)** | **8,5 s** (à vide) | 31,9 Mo | 15,4 Mo | 241 Mio |
| FP16 complet | 4K | 72 s | — | 56,7 Mo | 6,3 Mo | 1 291 Mio |
| Mixte | 4K | 175 s | — | 146,2 Mo | 19,2 Mo | 2 177 Mio |

Le moteur est lié à la carte, à la version de TensorRT et au pilote (ADR-043). Pour l'application : construction au premier lancement **≈ 1,5 min** en 720p à vide (jusqu'à 2,5 min si le PC est chargé ; à afficher à l'utilisateur), puis chargement du moteur sérialisé en **0,3–0,6 s** (`deserializeCudaEngine` + contexte, mesuré dans le banc). Un moteur par résolution d'interpolation, ou un profil dynamique (à mesurer) ; le cache de timing accélère fortement une reconstruction (nouveau pilote).

## P3.4 Vitesse et VRAM de RIFE seul

Banc `spike_bench rife --engine <moteur> [--size 3840x2160]` : pour chaque image générée, décalage image 1 → image 0 (copie D2D), conversion ARGB8 → planaire, `enqueueV3`, conversion planaire → ARGB8 ; **aucune synchronisation CPU dans la boucle** ; temps par événements CUDA (1 image sur 10). VRAM = delta de `QueryVideoMemoryInfo` du processus (moteur + contexte + tampons E/S). trtexec donne le calcul pur (graphes CUDA).

| Moteur | Résolution | trtexec ms (médiane) | Banc ms / image générée | Banc i/s | VRAM (Mio) |
|---|---|---|---|---|---|
| FP32/TF32 | 720p | 7,63 | 8,08 | 123–128 | 366 |
| FP16 complet | 720p | 4,44 | 5,10–5,20 | 194 | 238 |
| **Mixte** | **720p** | **5,21** | **5,60** | **181** | **354** |
| FP16 complet | 4K | 44,95 | 46,3 | 21,6 | 1 556 |
| Mixte | 4K | 58,50 | 58,6 | 17,0 | 2 698 |
| *rappel FRUC (phase 2)* | *720p* | — | *2,7 (synchro comprise)* | *≈ 365* | *98* |

4K ≈ 10,5–11 × 720p (9 × plus de pixels) : **RIFE à 4K est hors jeu** (17–22 i/s, 1,6–2,7 Gio).

## P3.5 Qualité des images générées

### Clips réels (clips QA du 2026-10-21, 720p, 60 → 30 → 60)

Méthode : vérité terrain 720p60 = `tests/clips/ref4k/<clip>.mkv` (FFV1 4K60) réduit par FFmpeg (`scale=1280:720:flags=lanczos,format=bgra`) ; SYN-TEXT : `in720/SYN-TEXT.mkv` (720p60). On garde les images paires (source 30 i/s), on génère chaque image impaire et on la compare à la vraie image impaire (299 images générées par clip) avec `libvmaf` (modèle par défaut v0.6.1, `feature=psnr|float_ssim`) du FFmpeg de mesure `C:\Tools\ffmpeg` (8.1.3 GPL). Script : `build/spike/scripts/p3_quality.sh` ; sortie brute : `build/spike/results/p3_quality.txt`. **Interpolation seule, sans SR ni encodage** ; les scripts `tests/quality/` de la QA mesurent la chaîne complète en 4K et ne s'appliquent pas tels quels à ce test.

| Clip | Méthode | VMAF moyen | VMAF moy. harmonique | PSNR-Y moyen (dB) | SSIM moyen | Images répétées |
|---|---|---|---|---|---|---|
| BBB-FAST (mouvements rapides, 1 coupure de plan) | duplication | 67,2 | 13,7 | 37,81 | 0,9520 | — |
| | FRUC | 89,2 | 40,7 | 43,86 | 0,9821 | 15 / 299 |
| | RIFE FP16 complet | 87,3 | 37,8 | 40,81 | 0,9867 | 0 |
| | RIFE FP32 | 91,5 | 38,5 | 47,08 | 0,9875 | 0 |
| | **RIFE mixte** | **91,5** | **38,7** | **47,08** | **0,9875** | 0 |
| BBB-PAN (panoramique, feuillages) | duplication | 55,6 | 47,5 | 29,82 | 0,9643 | — |
| | FRUC | 87,8 | 67,1 | 37,89 | 0,9944 | 0 |
| | RIFE FP16 complet | 84,0 | 64,7 | 36,31 | 0,9936 | 0 |
| | RIFE FP32 | 90,9 | 68,9 | 40,35 | 0,9959 | 0 |
| | **RIFE mixte** | **90,9** | **68,9** | **40,35** | **0,9959** | 0 |
| SYN-TEXT (texte défilant, synthétique) | duplication | 45,9 | 41,9 | 24,37 | 0,9633 | — |
| | FRUC | 55,0 | 39,6 | 24,52 | **0,9104** | 45 / 299 |
| | RIFE FP16 complet | 61,9 | 40,7 | 26,84 | 0,9503 | 0 |
| | RIFE FP32 | 69,7 | 54,4 | 27,55 | 0,9521 | 0 |
| | **RIFE mixte** | **69,7** | **54,4** | **27,55** | **0,9521** | 0 |

Lecture : RIFE (mixte = FP32) est **meilleur que FRUC sur les trois clips** : +2,3 VMAF (BBB-FAST), +3,2 (BBB-PAN), **+14,7 (SYN-TEXT)** ; sur le texte, FRUC fait **pire que la duplication en SSIM** (0,910 contre 0,963 : texte déformé) et il répète 5 à 15 % des images. Les VMAF minimaux à 0 (toutes méthodes) viennent des **coupures de plan** (l'image intermédiaire mélange deux plans) : à traiter par une détection de coupure (story à créer : répéter l'image au lieu d'interpoler sur une coupure). TOS-FACE / TOS-DARK (24 i/s) n'ont pas de vérité terrain intermédiaire : non mesurés ici.

### Mire synthétique (vérité terrain exacte, grain réglable)

`spike_bench interp-q --method <dup|fruc|rife> --noise <a> --motion <m>` : sources aux instants pairs avec un grain uniforme de ±a/2, image générée comparée à la mire **sans grain** à l'instant impair (PSNR RGB, 119 images). Sortie brute : `build/spike/results/p3_interpq.txt`.

| Grain (`--noise`) / mouvement | duplication | FRUC | RIFE FP16 complet | RIFE FP32 | RIFE mixte |
|---|---|---|---|---|---|
| 0 / ×1 | 17,9 dB | 38,1 dB | 41,4 dB | 44,4 dB | 44,4 dB |
| 0,02 (±1 %) / ×1 | 17,9 | **17,9 (119/119 répétées)** | 38,7 | 40,0 | — |
| 0,04 (±2 %) / ×1 | 17,9 | **17,9 (119/119 répétées)** | 36,3 | 36,7 | — |
| 0,08 (±4 %) / ×1 | 17,8 | **17,8 (119/119 répétées)** | 32,6 | 32,4 | — |
| 0 / ×2 | 16,0 | 36,3 | 38,4 | 39,6 | — |
| 0 / ×4 | 15,8 | 30,8 | 31,6 | 31,9 | — |

**Le défaut bloquant de FRUC (abandon total dès qu'il y a du grain) n'existe pas avec RIFE** : RIFE reste à 32–40 dB là où FRUC tombe au niveau de la duplication. (Une première série de ces mesures était faussée par une course entre le changement de grain et le flux CUDA non bloquant : corrigée, résultats invalides gardés à part dans `p3_interpq_race_invalide.txt`.)

## P3.6 Chaîne 720p30 → RIFE → VSR → NVENC 4K60

`spike_bench chain --order before --interp rife --engine v2_mixed_720.engine --q <q> --opts "<préréglage>"` (script `build/spike/scripts/p3_chain.sh`), même banc et même méthode qu'en P2.3 (source synthétique sur GPU, 300 images source → 599 images 4K, thread de traitement + thread NVENC, file bornée de 4). RIFE n'impose **aucune** synchronisation CPU (FRUC en imposait deux par image). Mesures du tableau : **CPU chargé** (compilation Qt) ; la répétition à vide est en P3.7.

| Préréglage | Interp. | VSR | Codec | Sortie 4K i/s | Interp. ms (méd.) | VSR ms | VRAM pic, delta (Mio) |
|---|---|---|---|---|---|---|---|
| Équilibré | **RIFE mixte** | 3 High | HEVC P5 | **106,4** | 5,55 | 2,65 | 1 549 |
| Équilibré | **RIFE mixte** | 4 Ultra | HEVC P5 | **104,5** | 5,67 | 3,29 | 1 549 |
| Équilibré | FRUC | 4 Ultra | HEVC P5 | 103,4 | 11,1* | 3,62 | 1 293 |
| Équilibré | **RIFE mixte** | 3 High | AV1 P5 | **109,7** | 5,78 | 2,70 | 1 946 |
| Équilibré | **RIFE mixte** | 4 Ultra | AV1 P5 | **108,6** | 5,61 | 3,41 | 1 946 |
| Qualité max | **RIFE mixte** | 4 Ultra | HEVC P7 HQ multipasse pleine | **36,9** | 5,45 | 3,36 | 1 945 |
| Qualité max | **RIFE mixte** | 4 Ultra | AV1 P7 HQ multipasse pleine | **43,6** | 5,43 | 3,39 | 2 138 |
| Rapide | RIFE mixte | 2 Medium | HEVC P3 | 222 | 5,48 | 1,57 | 936 |
| Rapide | FRUC | 2 Medium | HEVC P3 | 260 | 4,28* | 1,53 | 679 |
| Équilibré, **RIFE 4K après SR** | RIFE mixte 4K | 3 High | HEVC P5 | 32,4 | 57,5 | 10,9 | **3 949** |
| sans interpolation (30 → 30) | — | 4 Ultra | HEVC P5 | 96,7 | — | 3,38 | 1 197 |

\* FRUC : temps mural synchronisé, qui inclut l'attente du GPU partagé avec VSR et NVENC (2,7 ms quand FRUC tourne seul, phase 2) ; RIFE : événements CUDA.

Autres mesures de la phase 3 :
- RIFE FP16 complet dans la chaîne Équilibré HEVC q3 : 108,6 i/s ; RIFE FP32 q4 : 103,9 i/s / 1 559 Mio → **la précision de RIFE ne change pas le débit de la chaîne Équilibré** (NVENC reste le goulot).
- Équilibré HEVC q4 avec grain `--noise 0.04` (RIFE FP16) : 100,7 i/s, 0 image répétée.
- **Fuite** : 10 jobs RIFE + VSR + NVENC à la suite dans le même processus (`--repeat 10`) : **0 Mio de dérive** d'un job à l'autre (+40 Mio fixes au premier job : chargement des bibliothèques TensorRT/NGX, stable ensuite). `build/spike/results/p3_leak.txt`.
- **NVDEC** (FFmpeg de mesure, `-hwaccel cuda -hwaccel_output_format cuda`, H.264 720p `in720/BBB-FAST.mp4` lu 10 fois) : **≈ 3 100 i/s** (décodage logiciel : ≈ 1 300 i/s) → négligeable devant les 30 i/s nécessaires. Le FFmpeg LGPL de l'application (`third_party/ffmpeg/lgpl-shared/`) n'était pas encore posé.

Budget SM par image 4K de sortie (interpolation avant SR, 30 → 60) : ½ × 5,6 (RIFE mixte) + 3,3 (VSR q4) + 0,2 (conversions) ≈ **6,3 ms → ≈ 160 i/s côté SM** ; 24 → 60 (3/5 d'images générées) : ≈ 6,9 ms → ≈ 145 i/s. Le plafond reste NVENC en Équilibré (≈ 105 i/s) et en Qualité max (37–52 i/s) ; en **Rapide**, RIFE devient le facteur limitant (222 i/s contre 260 avec FRUC ; cible ≥ 100 : OK).

## P3.7 Mesures à vide

Compilation vcpkg terminée ; CPU à 10–37 % (autres applications du PO). `p3_chain.sh` répété (`build/spike/results/p3_chain_vide.txt`) ; VRAM identique au tableau P3.6.

| Préréglage | Interp. | VSR | Codec | Sortie 4K i/s **à vide** | (sous charge) |
|---|---|---|---|---|---|
| Équilibré | RIFE mixte | 3 High | HEVC P5 | **108,2** | 106,4 |
| Équilibré | RIFE mixte | 4 Ultra | HEVC P5 | **103,2** | 104,5 |
| Équilibré | FRUC | 4 Ultra | HEVC P5 | 104,0 | 103,4 |
| Équilibré | RIFE mixte | 3 High | AV1 P5 | **111,4** | 109,7 |
| Équilibré | RIFE mixte | 4 Ultra | AV1 P5 | **112,0** | 108,6 |
| Qualité max | RIFE mixte | 4 Ultra | HEVC P7 HQ | **36,5** | 36,9 |
| Qualité max | RIFE mixte | 4 Ultra | AV1 P7 HQ | **51,5** | 43,6 |
| Rapide | RIFE mixte | 2 Medium | HEVC P3 | **219** | 222 |
| Rapide | FRUC | 2 Medium | HEVC P3 | 267 | 260 |
| Équilibré, RIFE 4K après SR | RIFE mixte 4K | 3 High | HEVC P5 | 31,0 | 32,4 |
| sans interpolation | — | 4 Ultra | HEVC P5 | 96,7 | 96,7 |

La charge CPU ne change les résultats que de ±3 % (sauf AV1 Qualité max : +18 % à vide) : le banc est limité par les moteurs GPU, pas par le CPU. Écart avec la phase 2 (Équilibré FRUC HEVC q4 : 104 i/s) : nul. **Aucune régression** : RIFE donne le même débit de chaîne que FRUC en Équilibré et en Qualité max, −18 % en Rapide (219 contre 267 i/s, cible ≥ 100).

Fuite à vide avec le moteur mixte (`--repeat 10`, q4) : **0 Mio de dérive** (`p3_leak_mixed.txt`).

Construction du moteur mixte 720p **à vide** : **84,5 s** au premier lancement (sans cache), **8,5 s** avec le cache de timing (15,4 Mo).

## P3.8 Verdict et suites

- **RIFE 4.25 (TensorRT, précision mixte, 720p, avant la SR) remplace FRUC comme brique cible** : meilleure qualité sur les trois clips mesurables (+2 à +15 VMAF), pas d'abandon sur le grain, aucune synchronisation CPU, débit de chaîne identique (NVENC est le goulot), VRAM de chaîne 1,5–2,1 Gio (budget 9 Gio).
- **Interpolation après SR (4K) écartée** : 32 i/s, 3,9 Gio, RIFE 4K à 58 ms par image (ADR-044 confirmé).
- **Bloquant de livraison inchangé : licence des poids (SEC-03).** Éléments nouveaux pour secdevops : le README Practical-RIFE place les poids sous « la même licence MIT » ; l'ONNX vient d'un dépôt GPL-3.0 (vs-mlrt). Option la plus propre : **exporter nous-mêmes l'ONNX depuis les poids MIT d'origine** (exige Python + PyTorch cu128, non approuvé à ce jour), ou obtenir une confirmation de la licence des fichiers vs-mlrt.
- Suites techniques : détection des coupures de plan ; RIFE en 10 bits (le réseau travaille en RGB flottant : entrée P010 → RGB FP32 possible sans perte, à brancher avec le pipeline 10 bits) ; profil dynamique (une seule construction pour plusieurs résolutions) à mesurer ; construction du moteur via l'API C++ (`IBuilder` + `nvonnxparser`) au premier lancement, avec cache de timing ; mesurer RIFE 4.26 et une variante « lite » si la marge du préréglage Rapide doit grandir.
- Outils de la phase 3 (jetables, dans `build/spike/`) : `bench/main.cpp` (commandes `rife`, `interp-q`, `interp-file`, `chain --interp rife --engine`), `bench/synth.cu` (conversions ARGB8 ↔ planaire FP16/FP32, SSE), `rife/tools/onnx_fp16.cpp`, `scripts/p3_chain.sh`, `scripts/p3_quality.sh`, `scripts/runq.sh`. Sauvegardes de la phase 2 : `bench/*.phase2.bak`.
