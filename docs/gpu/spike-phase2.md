# Spike — Phase 2 (P2.1 à P2.6)

> Suite de [spike.md](spike.md) — extrait du fichier d'origine lors du ménage docs du 2026-10-29, texte inchangé.

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
