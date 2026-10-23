# Enhance — Architecture du pipeline vidéo GPU (proposition)

Statut : v0.2 du 2026-10-21 (gpu-engineer) — **contrat pipeline ↔ application aligné** avec `docs/architecture.md` v0.1 (§ 12, ADR-002, ADR-003, ADR-010) ; résumé et tableau des ADR dans `docs/architecture.md` § 13.
Entrées : `docs/vision.md` v0.2, `docs/backlog.md` v0.2 (décisions du PO n° 1 à 7), `docs/gpu/spike.md` (phase 1), `docs/design/ux/parcours.md` (P-08).

Historique : v0.1 (proposition, ADR numérotés P01–P11) → v0.2 : ADR renumérotés **ADR-040 à ADR-051** (plage réservée au pipeline), contrat public réécrit (§ 6), écarts avec l'application tranchés (§ 6.0).

**Avertissement (mis à jour après la phase 2 du spike, 2026-10-21).** Les SDK NVIDIA sont posés (RTX Video SDK 1.1.0, Video Codec SDK 13.1.15, Optical Flow SDK 5.0.7, TensorRT 11.4) et leurs en-têtes ont été lus : les points levés citent le fichier dans `docs/gpu/spike.md` § P2.2, et les chiffres mesurés sont marqués **[M]** (§ P2.3, entrées synthétiques). Restent **« à vérifier »** : FFmpeg (absent), NVDEC, TensorRT/RIFE, et toute la qualité objective. Tout chiffre encore marqué **[H]** reste une hypothèse.

Unités : pixels, kbit/s, secondes (`std::chrono`), VRAM en Mio (1 Gio = 1 024 Mio).

---

## 1. Vue d'ensemble

```
            CPU                                 GPU (RTX 5070, un seul contexte CUDA primaire)
 ┌──────────────────┐
 │ T0 Démux         │ paquets vidéo   ┌──────────────────────────┐
 │ libavformat +    ├────────────────►│ T1 Décodage              │  NVDEC via hwaccel FFmpeg (AV_PIX_FMT_CUDA)
 │ E/S Win32 (049)  │                 │                          │
 │                  │  file bornée    │  + conversion couleur    │  NV12/P010 → format pivot (noyau CUDA)
 │                  │                 │  + rotation / désentrel. │  flux S_dec
 │                  │                 └────────────┬─────────────┘
 │                  │                              │ VideoFrame (surface du pool source + événement CUDA)
 │                  │                 ┌────────────▼─────────────┐
 │                  │                 │ T2 Traitement            │  [Interpolation → 60 i/s si source < 50 i/s]
 │                  │                 │  chaîne d'IFrameProcessor│  → Super-résolution RTX Video SDK (→ 3840×2160)
 │                  │                 │                          │  → [SDR→HDR10, plus tard] → pivot → NV12/P010
 │                  │                 └────────────┬─────────────┘  flux S_proc
 │                  │                              │ VideoFrame (surface du pool 4K, déjà enregistrée NVENC)
 │                  │                 ┌────────────▼─────────────┐
 │                  │                 │ T3 Encodage NVENC        │  HEVC / AV1, 8 / 10 bits, HDR10 prévu
 │                  │                 │  (API Video Codec SDK)   │  mode asynchrone, événements Win32
 │                  │                 └────────────┬─────────────┘
 │                  │ paquets audio /              │ paquets compressés (mémoire CPU, quelques Mio/s)
 │                  │ sous-titres     ┌────────────▼─────────────┐
 │                  ├────────────────►│ T4 Mux libavformat       │  MP4 (bascule MKV), écrit au chemin exact
 └──────────────────┘  file bornée    │  copie sans réencodage   │  fourni par l'application (ADR-049)
                                      └──────────────────────────┘
 T5 Rapporteur : agrège compteurs et erreurs, appelle l'observateur de l'application (progression 4 fois/s, ADR-051).
```

Seuls transits CPU↔GPU : les **paquets compressés** (entrée et sortie, inévitables et petits) et, en **repli logiciel** uniquement, l'envoi des images décodées par le CPU (via mémoire épinglée). Aucune image ne revient sur le CPU dans le chemin nominal.

---

## 2. Décisions clés (ADR du pipeline)

Numérotation définitive dans la plage **ADR-040 et plus**, réservée au pipeline (`docs/architecture.md` en-tête) ; correspondance avec la v0.1 : P01→040, P02→041 … P11→050 ; ADR-051 est nouveau. Licences : à valider par secdevops dans `docs/security/licences.md` (ENH-006).

### ADR-040 — Décodage : NVDEC via **hwaccel FFmpeg** (`AV_HWDEVICE_TYPE_CUDA`)

| Option | Pour | Contre | Licence |
|---|---|---|---|
| **Retenue : libavcodec + hwaccel CUDA/NVDEC** (`get_format` → `AV_PIX_FMT_CUDA`, contexte CUDA partagé) | Un seul chemin de code pour tous les codecs ; FFmpeg gère analyse des flux, conteneurs, VFR, interlacé, données annexes (rotation, couleur, HDR) ; repli logiciel = même API ; utilise bien NVDEC matériel en dessous | Moins de contrôle sur le nombre de surfaces (réglé via `extra_hw_frames`) ; les surfaces appartiennent au pool FFmpeg → on copie GPU→GPU dans notre pool (≈ 1,4 Mio par image 720p NV12, négligeable) | FFmpeg **LGPL 2.1+**, liaison dynamique (build BtbN `lgpl-shared`) ; en-têtes `nv-codec-headers` **MIT** |
| Écartée : NVDEC direct (Video Codec SDK, `cuvidCreateVideoParser`/`cuvidDecodePicture`, classe d'exemple `NvDecoder`) | Contrôle total (surfaces, désentrelacement matériel, recadrage/redimensionnement à la sortie de NVDEC) | Il faut toujours libavformat + filtres de flux (`h264_mp4toannexb`…) ; repli logiciel = second chemin de code ; plus de code à maintenir | En-têtes Video Codec SDK (MIT, à vérifier) ; `nvcuvid.dll` fournie par le pilote |
| Écartée : décodeurs `h264_cuvid`/`hevc_cuvid` de FFmpeg | Désentrelacement NVDEC intégré | Chemin historique, moins maintenu que hwaccel ; noms de décodeurs par codec ; repli plus compliqué | LGPL |

Gardé ouvert : l'interface `IDecoder` permet de brancher plus tard un `NvdecDirectDecoder` si la phase 2 montre un gain mesuré.
À vérifier : `AV_CUDA_USE_PRIMARY_CONTEXT` dans `libavutil/hwcontext_cuda.h` (partage du contexte primaire avec TensorRT et NVENC).

### ADR-041 — Encodage : **NVENC direct** (API `nvEncodeAPI.h` du Video Codec SDK)

| Option | Pour | Contre | Licence |
|---|---|---|---|
| **Retenue : API NVENC native** (entrée `CUdeviceptr` enregistrée une fois, mode asynchrone Windows) | Contrôle complet : préréglages P1–P7, réglage de qualité (HQ ; **UHQ disponible en HEVC et AV1**, `nvEncodeAPI.h` l. 2225 — mesuré 29 / 38 i/s en 4K10 sur la 5070), anticipation (≤ 31 − B-frames), AQ temporelle, B-frames de référence, IDR forcée (`NV_ENC_PIC_FLAG_FORCEIDR`, l. 680), métadonnées HDR10 dans le flux (HEVC et AV1) ; `nvEncSetIOCudaStreams` (l. 4645) évite toute synchro CPU | Il faut construire nous-mêmes les paramètres du flux pour le mux (`nvEncGetSequenceParams` → `extradata`) ; plus de code | En-tête `nvEncodeAPI.h` (licence de l'en-tête à confirmer par secdevops sur `License.pdf` / `NOTICES.txt` du SDK 13.1.15) ; `nvEncodeAPI64.dll` fournie par le pilote, rien à redistribuer |
| Écartée : `hevc_nvenc` / `av1_nvenc` de libavcodec | Mux trivial, peu de code | Options dépendantes de la version FFmpeg, métadonnées HDR et réglages fins moins maîtrisés, une couche d'erreurs en plus | LGPL |
| Écartée : encodeurs logiciels | — | `libx265` est **GPL** (interdit, décision PO n° 4) ; SVT-AV1 (BSD) beaucoup trop lent en 4K60 | — |

### ADR-042 — Couleur et format pivot : **noyaux CUDA maison**, conversions explicites

- Entrée NVDEC : NV12 (8 bits) ou P010 (10 bits, aussi pour les sources 12 bits ramenées à 10). Repli logiciel : `yuv420p`, `yuv422p10`, etc. → envoi via mémoire épinglée puis même noyau.
- **Format pivot** entre étages de traitement : RGB **demi-flottant (FP16)** à la résolution source (précision 10 bits sans effort, entrée naturelle de RIFE) ; à la sortie de la super-résolution, le format **natif du RTX Video SDK**. **Levé en phase 2** : le SDK a une **interface CUDA** native, mais n'accepte que du **RGB** : `CUarray` ARGB 8 bits ou **ABGR10** (`CU_AD_FORMAT_UNORM_INT_101010_2`) lu par `CUtexObject`, sortie écrite par `CUsurfObject` dans le même format (FP16 seulement en sortie TrueHDR). Le pivot devant la SR est donc **ABGR10 en `CUarray`** (10 bits conservés, mesuré aussi rapide qu'en 8 bits) ; noyaux maison NV12/P010 → ABGR10 avant, ABGR10 → P010 après (0,1 ms en 4K **[M]**). Le FP16 reste le pivot de RIFE si RIFE est retenu (conversion ABGR10 ↔ FP16 à 720p, négligeable).
- Conversions avec **matrice, primaires, fonction de transfert et plage** lues dans le flux (`AVFrame::colorspace/color_primaries/color_trc/color_range`), avec règles par défaut testées : SD (≤ 576 lignes) non étiqueté → BT.601 ; sinon BT.709 ; plage limitée par défaut sauf `AVCOL_RANGE_JPEG`. Sortie SDR : BT.709 plage limitée, étiquetée explicitement dans le flux et le conteneur. Sortie HDR10 (plus tard) : BT.2020 non constant + PQ, P010.
- La **rotation** (matrice d'affichage) est appliquée physiquement dans le noyau de conversion (90/180/270°) ; le **SAR** est corrigé ; la sortie garde le rapport d'aspect dans 3840×2160 (ex. 4:3 → 2880×2160), sans bandes noires.
- Écartées : `scale_cuda`/`zscale` de libavfilter (dépendance aux options de build FFmpeg, contrôle limité de la plage 10 bits, `zscale` sur CPU) ; NPP (CUDA, redistribuable, mais couverture partielle BT.2020/plages 10 bits et une DLL lourde de plus). Licence de la solution retenue : notre code.
- Tests : fonctions de conversion écrites en `__host__ __device__` → les mêmes formules sont testées **sur CPU en CI** (images de mires, valeurs connues BT.709/BT.2020, aller-retour YUV→RGB→YUV à ±1 code près).

### ADR-043 — Brique d'interpolation : **RIFE 4.x + TensorRT natif** (cible), **FRUC Optical Flow** en repli

| Option | Qualité | Vitesse 720p (RTX 5070) | Licence | Statut |
|---|---|---|---|---|
| **RIFE 4.25 (ONNX « v2 ») → moteur TensorRT 11 en précision mixte** (convolutions FP16, grilles de déformation FP32 — TensorRT 11 n'a plus de `--fp16`, la précision vient de l'ONNX), E/S en `CUdeviceptr` | **Meilleure, mesurée [M]** : VMAF des images générées 91,5 / 90,9 / 69,7 (BBB-FAST / BBB-PAN / SYN-TEXT) contre 89,2 / 87,8 / 55,0 pour FRUC ; aucun abandon sur grain. FP16 complet écarté (−4 à −8 VMAF) | **5,6 ms / image générée (181 i/s) en 720p, 354 Mio [M]** ; 4K : 58,6 ms, 2,7 Gio | Code RIFE **MIT** ; poids annoncés MIT par le README Practical-RIFE, ONNX issu de vs-mlrt (dépôt GPL-3.0) → **avis secdevops bloquant (SEC-03)** ; TensorRT : licence NVIDIA, redistribution des DLL à vérifier | **Retenue (cible)** sur mesures (spike phase 3) ; livraison **conditionnée** à la licence des poids |
| NVIDIA Optical Flow SDK 5.0.7 – FRUC (`NvOFFRUC.dll`, `NvOFFRUC/Interface/NvOFFRUC.h`) | Inférieure (halos, occlusions) ; **8 bits seulement** (NV12/ARGB) ; **sur grain ≥ ±2 %, répète l'image précédente au lieu d'interpoler** **[M]** | **2,7 ms / image générée en 720p (≈ 365 i/s), 22,7 ms en 4K [M]** ; 98 Mio en 720p | Licence SDK NVIDIA ; redistribuer `NvOFFRUC.dll` + `cudart64_110.dll` (à valider secdevops) | **Fonctionne sur Blackwell** via JIT PTX (code `sm_75/80` seulement) ; fuite +2 Mio par session ; **sur clips réels : répète 5–15 % des images, déforme le texte (SSIM 0,910 < duplication 0,963 sur SYN-TEXT) [M]** ; repli **uniquement** si la licence RIFE est refusée, et à condition que la QA accepte son comportement sur grain |
| Écartée : RIFE via ONNX Runtime + fournisseur TensorRT | = RIFE | Un peu moins | ONNX Runtime **MIT** | Une dépendance lourde de plus, contrôle mémoire et flux CUDA moins direct ; gardé en secours si l'intégration TensorRT native coince |
| Écartée : FFmpeg `minterpolate` | Faible | CPU, lent | LGPL | Point bas de comparaison VMAF seulement |

Le moteur TensorRT est propre à la carte et à la version de TensorRT : construit **localement au premier lancement** (sans réseau) depuis l'ONNX livré, mis en cache dans `%LOCALAPPDATA%\Enhance\cache\trt\` avec une clé (version TensorRT, `sm_120`, version pilote, empreinte de l'ONNX). Durée de construction **mesurée [M] : 84,5 s** pour le moteur mixte 720p au premier lancement (144 s avec le CPU chargé), **8,5 s** avec le cache de timing (15,4 Mo, à conserver avec le moteur) ; chargement d'un moteur en cache : 0,3–0,6 s. Affichée à l'utilisateur. La conversion de précision (ONNX FP32 → mixte) est faite une fois à la préparation du modèle, pas dans l'application. Détails et commandes : `docs/gpu/spike.md` phase 3.

### ADR-044 — Ordre : **interpolation avant la super-résolution** (provisoire, tranché par mesure)

Coût GPU par seconde de vidéo 30 → 60 i/s (hors encodeur, matériel séparé) :
- **Avant SR** : `60 × t_SR + 30 × t_RIFE(720p)`
- **Après SR** : `30 × t_SR + 30 × t_RIFE(4K)`, avec `t_RIFE(4K) ≈ 9 × t_RIFE(720p)` **[H]**

L'avant-SR **n'est donc pas gratuit** : il double le nombre d'images à agrandir. Il gagne si `t_SR < 8 × t_RIFE(720p)` environ. Retenu provisoirement pour : (1) VRAM — RIFE à 4K coûte 2–4 Gio **[H]** et ferait sortir du budget de 9 Gio (§5) ; (2) les flux optiques de RIFE sont plus fiables à basse résolution sur les grands mouvements ; (3) les images 4K passent toutes par la SR, rendu homogène. Risque : la SR peut aussi agrandir les défauts des images générées. **La décision finale est prise par l'ADR d'ENH-002 sur VMAF (images générées et vidéo entière) + i/s + VRAM, sur `REF-A`.** Les deux ordres sont possibles sans réécriture : la chaîne d'étages est une liste configurable (§3.3).

**Mesures du spike (phase 3, 2026-10-21) [M]** : `t_RIFE(4K) ≈ 10,5 × t_RIFE(720p)` (58,6 ms contre 5,6 ms, moteur mixte) et `t_SR` = 2,7–3,4 ms (q3–q4) : l'avant-SR gagne largement (`t_SR < 8 × t_RIFE(720p)` = 45 ms). Chaîne Équilibré HEVC : **avant SR 103–108 i/s, 1,5 Gio** ; **après SR 31 i/s, 3,9 Gio**. L'ordre « avant SR » est **confirmé** pour la vitesse et la VRAM ; il reste à vérifier par la QA que la SR n'amplifie pas visiblement les défauts des images générées (VMAF de la vidéo 4K entière).

Cas particuliers (décision PO n° 1) : source ≥ 50 i/s → pas d'interpolation, cadence conservée ; < 50 i/s (y compris 31–49) → 60 i/s constants. Proportion d'images générées : 30→60 = 1/2, 24→60 = 3/5, 25→60 = 11/12 → le coût de l'interpolation dépend de la source (pris en compte dans l'ETA).

### ADR-045 — Concurrence : **un thread par étage + flux CUDA + événements**, files bornées

| Option | Statut |
|---|---|
| **Retenue** : 5 threads (démux, décodage, traitement, encodage, mux) + 1 thread rapporteur ; un flux CUDA par étage GPU ; passage d'images par `VideoFrame` portant un **événement CUDA** ; l'étage suivant fait `cuStreamWaitEvent` (attente côté GPU, pas côté CPU) | Simple à raisonner, recouvre NVDEC / SM / NVENC (trois moteurs matériels distincts), pas de `cudaStreamSynchronize` dans la boucle |
| Écartée : boucle mono-thread | Pas de recouvrement : décodage, SR et encodage s'additionnent |
| Écartée : pool de tâches générique + graphes CUDA | Complexité inutile pour un seul job à la fois ; graphes CUDA à réévaluer en optimisation si la mesure montre un coût de lancement de noyaux |

### ADR-046 — Pause : **VRAM conservée** (MVP)

La pause gèle les étages entre deux images (reprise immédiate, < 2 s, aucune image perdue). La VRAM du job (~6–7 Gio **[H]**) reste allouée. Libérer la carte pendant une pause (ex. pour jouer) exige de tout détruire et de reprendre à un point précis : cela réutilise la mécanique de **reprise après plantage** (ENH-038), donc plus tard. **Point à faire valider par le PO** (ENH-021 dit « selon ADR »).

### ADR-047 — Erreurs : **`enh::Expected<T, PipelineError>`** (alias de l'ADR-003), aucune exception aux frontières

- **Tranché** : le pipeline n'a **pas** son propre type. Il utilise l'alias unique `enh::Expected<T, E>` de l'ADR-003 (= `tl::expected`, **CC0-1.0** ; passage à `std::expected` le jour du C++23, une ligne à changer). L'en-tête de l'alias vit dans une petite cible commune **`enh_common`** (`src/common/include/enh/expected.hpp`, en-têtes seuls), liée à la fois par `enh_core` et par le pipeline (le pipeline ne peut pas dépendre de `enh_core`, c'est l'inverse). L'ancien `enh::Result<T>` de la v0.1 disparaît de l'API publique ; dans l'implémentation, `template <class T> using Result = enh::Expected<T, PipelineError>;` est un simple raccourci local (même type, aucune conversion).
- Raison de s'aligner : un seul type traverse la frontière, le cœur teste les erreurs du pipeline sans adaptateur, et aucune raison forte ne justifie un type différent.
- Chaque code retour C (`CUresult`, `cudaError_t`, `NVENCSTATUS`, `int` FFmpeg, codes du RTX Video SDK, statuts TensorRT, `GetLastError()` Win32) passe par une fonction `check(...)` qui produit un `PipelineError` avec le code P-08 correspondant (§ 6.6), le code natif, le message natif (`cuGetErrorString`, `av_strerror`…) et `std::source_location` (journal). Le pipeline ne produit **aucun texte destiné à l'utilisateur** : des codes et des données, la présentation traduit (ADR-003, ADR-008).
- Exceptions de bibliothèques tierces (TensorRT peut en lever, `std::bad_alloc`) : attrapées au bord de l'étage, converties en `PipelineError` ; un `noexcept` sur chaque fonction publique et chaque rappel garantit qu'aucune exception ne sort.

### ADR-048 — DLL NVIDIA chargées **dynamiquement**

L'application doit démarrer sur un PC sans carte compatible (ENH-031, mode lecture seule). Donc : `cudart` lié **statiquement** (autorisé par l'EULA CUDA, rien à redistribuer ; il charge `nvcuda.dll` à la demande) ; NVENC via `NvEncodeAPICreateInstance` chargé avec `LoadLibraryExW(..., LOAD_LIBRARY_SEARCH_SYSTEM32)` ; TensorRT, RTX Video SDK, FRUC en **`/DELAYLOAD`** depuis le dossier de l'application uniquement (`SetDefaultDllDirectories`) — protection contre le détournement de DLL (point secdevops).

**Amendement 1 (ENH-040, 2026-10-23, gpu-engineer).** Mesuré et décidé à l'implémentation :
1. **RTX Video SDK : chargement explicite, pas `/DELAYLOAD`.** `enh_rtxvideo_bridge.dll` (ADR-037) est chargée par `LoadLibraryExW(<dossier de l'exe>\enh_rtxvideo_bridge.dll, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)` puis `GetProcAddress` sur les 7 fonctions `rvb_*` et contrôle de `rvb_abi_version()` (classe RAII `BridgeLibrary`). Raisons : chemin absolu maîtrisé ; absence ou ABI différente rendues en `Expected` (`ComponentMissing{RtxVideoSdk}`) au lieu d'une exception SEH du chargeur différé ; aucune bibliothèque NGX liée à l'application (`nvsdk_ngx_d.lib` retirée de `NVIDIA::rtx_video`). Écarté : `/DELAYLOAD` (échec = exception SEH à attraper par `__try`, pas de contrôle de version).
2. **DLL du pilote préchargées depuis System32** avant tout usage CUDA / FFmpeg / NGX : `nvcuda`, `nvcuvid`, `nvEncodeAPI64`, `nvofapi64`, `nvml` (`preloadNvidiaDriverDlls`). Le pont est lié avec `/DEPENDENTLOADFLAG:0x800`.
3. **NGX n'est pas un chargeur ordinaire** (constaté, pilote 617.42, journal NGX) : son cœur `_nvngx.dll` vient du DriverStore, ses modules sont projetés sans le chargeur Windows (invisibles à `EnumProcessModules`), la DLL de fonction exécutée peut être une copie rangée dans `%ProgramData%\NVIDIA\NGX\models`, et NGX lance `nvngx_update.exe -api update … -bootstrap` à chaque initialisation, **même avec l'identifiant d'application 0 et sans appel de mise à jour**. D'où, au lieu du contrôle des modules prévu par la note d'ENH-040 :
   - **création de processus enfants interdite** pendant l'initialisation de NGX et la création de l'effet (Job Object, `JOB_OBJECT_LIMIT_ACTIVE_PROCESS = 1`, limite levée ensuite ; RAII `ChildProcessBlocker`) : `nvngx_update.exe` ne peut pas démarrer depuis Enhance ;
   - **DLL de fonction vérifiée par son contenu** : NGX annonce dans son journal (niveau ON, gardé en mémoire, jamais écrit) le chemin de la DLL de fonction utilisée ; elle doit être `nvngx_vsr.dll` livrée (signée NVIDIA, vérifiée hors ligne) ou une copie identique octet pour octet, sinon `ComponentMissing` (`require_shipped_snippet`, vrai par défaut).
   Écartés : énumération des modules (inopérante) ; écriture dans le registre du pilote pour couper les mises à jour (touche le système de l'utilisateur, non documenté). Preuve réseau : ENH-041.

### ADR-049 — Fichiers : **l'application possède le cycle de vie**, le pipeline écrit au chemin exact reçu, E/S FFmpeg par **`AVIOContext` Win32 maison**

**Partage des rôles (tranché, aligné sur l'ADR-010 ; remplace le `.enhance-part` de la v0.1)** :

| Qui | Fait quoi |
|---|---|
| Application (`OutputPathPlanner`, `Orchestrator`) | Nom final (`_Enhance_4K60`, `_2`, `_3`…), extension (prise dans `OutputPlan::extension`), nom du temporaire `<nom final>.enhance-tmp.<ext>`, renommage final `MoveFileExW` sans remplacement, suppression du temporaire après annulation / échec (à l'expiration de « Rétablir »), nettoyage au démarrage des seuls temporaires référencés dans `queue.json`, vérification de l'espace disque avant. |
| Pipeline | Crée **exactement** `JobSpec::output_path` en `CREATE_NEW` (jamais d'écrasement : si le fichier existe → `OutputExists`, l'application choisit un autre nom) ; écrit ; en cas de succès, écrit la fin du conteneur, `FlushFileBuffers`, **ferme le fichier avant** `onFinished` ; en cas d'annulation ou d'erreur, **ferme** le fichier sans rien supprimer ni renommer. Le format du conteneur est passé explicitement à FFmpeg (`avformat_alloc_output_context2(…, "mp4" | "matroska", …)`), jamais déduit de l'extension, donc le suffixe `.enhance-tmp.mp4` est sans effet. |

Raisons : un seul propriétaire pour le nettoyage (pas de double suppression ni de course avec le délai « Rétablir » de 5 s) ; le temporaire est connu de `queue.json` donc nettoyable après un plantage ; le pipeline reste sans état sur le disque.

**Chemins longs / Unicode (tranché)** : le pipeline n'utilise **pas** le protocole `file:` de FFmpeg. Entrée et sortie passent par un `AVIOContext` personnalisé (`avio_alloc_context` + rappels lecture / écriture / déplacement sur un `HANDLE` Win32, classe RAII `Win32File`), ouvert par `CreateFileW` sur la forme étendue du chemin : `std::filesystem::absolute(p)` puis préfixe `\\?\` (ou `\\?\UNC\serveur\partage\…` pour un chemin UNC). Aucun passage par UTF-8, aucune limite à 260 caractères, lecteurs réseau compris.
- Entrée : `GENERIC_READ`, partage `FILE_SHARE_READ`, `FILE_FLAG_SEQUENTIAL_SCAN` ; `avformat_open_input(&ctx, "", fmt, &opts)` avec `ctx->pb` déjà posé et le drapeau `AVFMT_FLAG_CUSTOM_IO`.
- Sortie : `GENERIC_READ | GENERIC_WRITE` (le déplacement de l'index MP4 en tête, `faststart`, relit le fichier : à vérifier dans `libavformat/movenc.c`, fonction de décalage des données), `CREATE_NEW`, partage `FILE_SHARE_READ`.
- **Rappel `io_open` remplacé** sur les contextes d'entrée et de sortie : il refuse toute ouverture secondaire (listes HLS/DASH, `concat`, références externes MOV, protocoles réseau), sauf la réouverture du fichier de sortie lui-même demandée par `faststart`. Effet de bord voulu : **aucun accès réseau ni fichier hors des deux chemins du job**, quelle que soit la source (argument secdevops, ENH-028, ENH-015).
- Erreurs Win32 traduites précisément (grâce aux rappels maison) : `ERROR_FILE_NOT_FOUND` / `ERROR_PATH_NOT_FOUND` → `SourceMissing` ; `ERROR_ACCESS_DENIED` / `ERROR_SHARING_VIOLATION` en entrée → `SourceUnreadable`, en sortie → `WriteDenied` ; `ERROR_DISK_FULL` / `ERROR_HANDLE_DISK_FULL` → `DiskFull` ; `ERROR_FILE_EXISTS` → `OutputExists` ; disparition d'un lecteur réseau en cours de lecture → `SourceMissing`.
- Écartées : chemins UTF-8 donnés à FFmpeg (le protocole `file:` convertit en UTF-16 et ajoute `\\?\` aux chemins longs dans les versions récentes — `libavutil/wchar_filename.h`, à vérifier — mais on perd le contrôle des ouvertures secondaires, du mode de partage, de `CREATE_NEW` et de la précision des erreurs) ; chemins courts 8.3 (`GetShortPathNameW`, désactivables par volume, inopérants en écriture). Licence : notre code (API Win32 système).
- Test commun avec le developer (risque A6) : accents, espaces, emoji, chemin > 260 caractères, UNC, fichier verrouillé, disque plein simulé (fakes en CI ; volume réel saturé en local).

### ADR-050 — Conteneur : **MP4, bascule MKV** (décision PO n° 2)

Règle appliquée **avant** le traitement par `plan()` (§6.3), pour que l'application affiche le message :
1. Pour chaque piste audio : compatible MP4 si `avformat_query_codec(mp4, codec_id, FF_COMPLIANCE_NORMAL) == 1` (à vérifier) — sinon MKV.
2. Sous-titres : `mov_text` → MP4 ; SRT / WebVTT → MP4 avec **conversion en `mov_text`** (texte conservé, aucun style perdu car il n'y en a pas) ; ASS/SSA (styles), PGS, VobSub, DVB (images) → **MKV**.
3. Pièces jointes MKV (polices) → MKV.
4. Chapitres, titres et langues des pistes copiés dans les deux cas.
Point au PO : la conversion SRT → `mov_text` est un réencodage de texte, sans perte du texte ; à confirmer qu'elle respecte l'esprit « sans réencodage ».

### ADR-051 — Contrat d'API pipeline ↔ application (`JobHandle` RAII, observateur, progression, `plan()` pur)

Décisions (détail et code au § 6) :
1. **Deux interfaces sans GPU pour l'avant-traitement** : `IMediaProbe` (`probe`, `thumbnail`), CPU seulement, appelables depuis plusieurs threads ; `plan()` est une **fonction libre pure** (`noexcept`, aucune E/S, aucun appel GPU) qui prend `MediaInfo`, `PipelineParams` et un instantané `GpuCapabilities` (simple donnée). Un objet `IPipeline` n'est nécessaire que pour `capabilities()` et `start()`.
2. **`JobHandle`** : classe valeur **déplaçable, non copiable** (pimpl sur `std::unique_ptr<detail::IJobControl>`), plutôt que `std::unique_ptr<IJob>` exposé : l'application manipule un objet simple dont le destructeur garantit l'arrêt (annulation + attente si le job tourne encore), sans voir de hiérarchie virtuelle. Le faux pipeline renvoie le même type.
3. **Observateur passé par référence** (`IJobObserver&`, pas de `shared_ptr`) : durée de vie garantie par la règle « l'observateur survit au `JobHandle` », et le destructeur du `JobHandle` attend la fin du dernier rappel.
4. **Rappels** : `noexcept`, émis par **un seul thread du pipeline** (rapporteur T5), sérialisés, non bloquants (< 1 ms), **sans appel en retour vers le pipeline** depuis un rappel (règle de l'ADR-002 ; détruire le `JobHandle` ou appeler `wait()` depuis un rappel = interblocage, détecté par assertion en debug).
5. **Progression émise 4 fois/s** (période 250 ms), ce qui respecte le plafond de 10 Hz de l'application et suffit à son rafraîchissement de 500 ms ; changements d'étape et d'état émis immédiatement, hors cadence. L'ETA n'est **pas** calculée par le pipeline : elle appartient à l'`EtaEstimator` du cœur (ADR-004), qui reçoit les compteurs bruts — pas de double lissage.
6. **`start()` ne bloque pas** (< 50 ms : validation et lancement des threads) ; ouverture des fichiers, contrôle de VRAM, construction des modèles se font ensuite, et leurs erreurs arrivent par `onFinished`.
7. **Codes d'erreur calqués sur P-08** (une ligne du tableau P-08 = un code), plus quelques codes hors P-08 explicitement rangés (§ 6.6).

Écartés : `std::unique_ptr<IJob>` + `shared_ptr<IJobObserver>` de la v0.1 (deux propriétés partagées à raisonner, destruction implicite dans un thread du pipeline) ; rappels autorisés à appeler `pause()`/`cancel()` (inutile puisque l'application reposte tout vers son thread, et source d'interblocages subtils) ; progression à 10 Hz (2,5 fois plus de messages pour rien) ; ETA calculée dans le pipeline (doublon avec le cœur). Licence : notre code.

### ADR-052 — Pilote NVIDIA minimal : **R610 (610.00)** pour l'application, **617.42** comme pilote de référence des mesures

| Composant (version épinglée) | Pilote Windows minimal | Source (dans `third_party/nvidia/` ou le Toolkit installé) |
|---|---|---|
| **Video Codec SDK 13.1.15** (API NVENC 13.1, `NVENCAPI_MINOR_VERSION 1`) | **610 et plus** — contrainte dominante | `video-codec-sdk/Read_Me.pdf` § 1.3 « System Requirements » (« Driver version 610 and above », Windows et Linux) ; `video-codec-sdk/Interface/nvEncodeAPI.h` l. 114-115 |
| CUDA 13.4 (`cudart` statique, code `sm_120` natif) | r580 (plancher de compatibilité mineure CUDA 13.x, documentation CUDA en ligne) | **Non vérifiable localement** : le `CUDA_Toolkit_Release_Notes.txt` livré avec la v13.4 contient le texte de la 11.8 (périmé). Sans effet sur la décision : couvert par 610 |
| RTX Video SDK 1.1.0 | r550.58 (VSR), **r570** pour TrueHDR en CUDA | `rtx-video-sdk/doc/NVIDIA_RTX_Video_SDK_Programming_Guide.pdf` § 2.1 ; `rtx-video-sdk/samples/ReadMe.md` l. 5 (« R570.20 or later ») |
| TensorRT 11.4.0.106 (CUDA 13.4) | aucune valeur dans `tensorrt/doc/README.txt` ; suit CUDA 13.x | — |
| Blackwell `sm_120` (RTX 50) | pilotes RTX 50 à partir de la branche r570 | couvert par 610 |

Décision :
- **Minimum de l'application = 610.00** (branche R610). En dessous, le pilote n'expose pas l'API NVENC 13.1 (ouverture de session refusée, `NV_ENC_ERR_INVALID_VERSION`) : sans encodeur, Enhance ne peut rien produire. C'est la valeur à afficher en P-01 (« pilote NVIDIA 610 ou plus récent ») et à contrôler au démarrage (ENH-031, mode lecture seule si inférieur). Le contrôle porte sur la version du pilote (NVML `nvmlSystemGetDriverVersion`) **et** sur la réponse réelle des API (`NvEncodeAPIGetMaxSupportedVersion` ≥ 13.1 ; indicateur « pilote à mettre à jour » de NGX pour VSR / TrueHDR), pour ne pas dépendre d'un seul chiffre.
- **Minimum des tests GPU du poste de dev = même plancher 610.00** (`scripts/gpu-requirements.json`, `minDriver`, bloquant) : un poste au minimum doit pouvoir lancer les tests, sinon on ne prouve jamais que ce minimum fonctionne. **Pilote de référence des mesures = 617.42** (`referenceDriver`, informatif) : les comparaisons de vitesse et de qualité entre `main` et une branche (règle des 5 %) ne valent qu'à pilote identique ; un changement de pilote invalide aussi les moteurs TensorRT sérialisés et le cache de timing (ADR-043).
- Écartés : 580 (P-01 actuel : ne couvre pas NVENC 13.1) ; 550 / 570 (RTX Video SDK seul, même défaut) ; 617.42 comme minimum (pilote du poste du PO, aucune exigence technique ne l'impose : exclurait sans raison les utilisateurs en R610).
- À vérifier (QA, une fois) : chaîne complète sur un pilote de la branche R610 (seul 617.42 a été testé). Si un composant (TensorRT 11.4 notamment) exige davantage, cet ADR est révisé et la valeur remontée.

---

## 3. Conception interne

### 3.1 Modules et cibles CMake

```
src/common/include/enh/expected.hpp   alias enh::Expected (ADR-003), cible enh_common partagée avec l'application
src/pipeline/include/enh/pipeline/  en-têtes publics (cible enh_pipeline_api) : seul point d'entrée du developer (§6)
src/pipeline/plan/    plan() pur + ContainerPolicy + tables d'estimation (cible enh_pipeline_plan, CI)
src/pipeline/fake/    FakePipeline, FakeMediaProbe (cible enh_pipeline_fake, CI, utilisée par les tests de l'app)
src/pipeline/core/    C++20 pur, compilé et testé en CI : Pipeline, BoundedQueue, PauseGate,
                      CadencePlanner, ColorResolver, VramBudget, Reporter (cadence 250 ms)
src/pipeline/ffmpeg/  RAII FFmpeg + Win32File/AVIO maison (ADR-049) + FfmpegDemuxer, FfmpegDecoder
                      (hwaccel + logiciel), FfmpegMuxer, FfmpegMediaProbe (probe + vignette)
                      (compilé en CI : démux, mux, sondage et décodage logiciel testables sans GPU)
src/pipeline/cuda/    RAII CUDA, pools de surfaces, noyaux de couleur (.cu, sm_120 + PTX compute_120)
src/pipeline/nvenc/   NvencEncoder
src/pipeline/rtxvideo/RtxVideoUpscaler (+ RtxVideoHdrConverter plus tard)
src/pipeline/interp/  RifeTrtInterpolator, FrucInterpolator
src/pipeline/gpu_factory/ NvidiaStageFactory (assemble les implémentations réelles)
tests/unit/pipeline/  fakes + tests CI ;  tests/gpu/pipeline/  tests et bancs sur la RTX
```
Option CMake `ENHANCE_WITH_GPU` (OFF en CI) : sans elle, les dossiers `cuda/ nvenc/ rtxvideo/ interp/` ne sont pas compilés et `createNvidiaPipeline()` renvoie `ComponentMissing{NvidiaGpu}`. `enh_pipeline_api`, `enh_pipeline_plan`, `enh_pipeline_fake` et `ffmpeg/` ne dépendent ni de CUDA ni des SDK NVIDIA.

### 3.2 Interfaces internes (testables sans GPU)

```cpp
namespace enh::pipeline {

// Image en transit : déplaçable, non copiable. La surface retourne au pool à la destruction (RAII).
struct VideoFrame {
  FrameDesc desc;                 // largeur/hauteur (px), PixelFormat, ColorInfo
  std::int64_t pts;               // dans time_base
  Rational time_base;
  std::int64_t index;             // numéro d'image (source ou sortie)
  FrameFlags flags;               // SceneCut, Generated, Duplicated
  PooledSurface surface;          // mémoire GPU (réel) ou CPU (fakes)
  GpuFence ready;                 // événement CUDA : données valides après ; vide pour les fakes
};

class IFrameSink {               // l'étage suivant (une file bornée en pratique)
 public:
  virtual ~IFrameSink() = default;
  virtual Result<void> push(VideoFrame&& f, std::stop_token st) = 0;  // bloque si plein (back-pressure)
};

class IDemuxer {
 public:
  virtual ~IDemuxer() = default;
  virtual const MediaInfo& info() const = 0;
  virtual Result<std::optional<Packet>> read() = 0;                    // nullopt = fin
  virtual Result<void> seek(std::chrono::duration<double> t) = 0;     // aperçu, reprise
};

class IDecoder {
 public:
  virtual ~IDecoder() = default;
  virtual Result<void> decode(const Packet* pkt, IFrameSink& out, std::stop_token st) = 0; // nullptr = vidange
  virtual DecoderPath path() const = 0;                                // Nvdec | Software (journal)
};

// N images en entrée → M en sortie (interpolation : M ≥ N ; SR, couleur, HDR : M = N).
class IFrameProcessor {
 public:
  virtual ~IFrameProcessor() = default;
  virtual Result<FrameDesc> configure(const FrameDesc& in, const ProcessorSettings& s) = 0;
  virtual Result<void> process(VideoFrame&& in, IFrameSink& out, std::stop_token st) = 0;
  virtual Result<void> flush(IFrameSink& out, std::stop_token st) = 0;
  virtual VramEstimate vramEstimate(const FrameDesc& in) const = 0;   // contrôle avant démarrage
};
class IInterpolator : public IFrameProcessor {};   // marqueurs pour la fabrique et les tests
class IUpscaler     : public IFrameProcessor {};
class IHdrConverter : public IFrameProcessor {};   // plus tard (ENH-032)

class IEncoder {
 public:
  virtual ~IEncoder() = default;
  virtual Result<StreamParameters> configure(const FrameDesc& in, const EncodeSettings& s) = 0; // extradata pour le mux
  virtual Result<void> encode(VideoFrame&& f, IPacketSink& out, std::stop_token st) = 0;
  virtual Result<void> flush(IPacketSink& out) = 0;
};

class IMuxer {
 public:
  virtual ~IMuxer() = default;
  virtual Result<void> open(const std::filesystem::path& output_path, Container c,   // CREATE_NEW (ADR-049)
                            const StreamParameters& video, std::span<const CopiedStream> copied) = 0;
  virtual Result<void> write(Packet&& p) = 0;     // entrelacement par dts (av_interleaved_write_frame)
  virtual Result<void> finalize() = 0;            // fin du conteneur + FlushFileBuffers + fermeture (pas de renommage)
  virtual void abort() noexcept = 0;              // ferme le fichier ; ne supprime rien (l'application s'en charge)
};

class IStageFactory {             // injectée dans le moteur : réelle (NVIDIA) ou fausse (tests)
 public:
  virtual ~IStageFactory() = default;
  virtual Result<GpuCapabilities> capabilities() = 0;
  virtual Result<std::unique_ptr<IDemuxer>> demuxer(const std::filesystem::path&) = 0;
  virtual Result<std::unique_ptr<IDecoder>> decoder(const MediaInfo&) = 0;
  virtual Result<std::unique_ptr<IInterpolator>> interpolator(InterpolationQuality) = 0;
  virtual Result<std::unique_ptr<IUpscaler>> upscaler(SuperResQuality) = 0;
  virtual Result<std::unique_ptr<IEncoder>> encoder(VideoCodec) = 0;
  virtual Result<std::unique_ptr<IMuxer>> muxer() = 0;
  virtual Result<VramInfo> vram() = 0;
};
}
```

**Fakes pour la CI** (`tests/unit/pipeline/fakes/`) : `FakeDemuxer` (paquets synthétiques, VFR, coupures de plan, flux audio/sous-titres factices, erreurs injectées à l'image N), `FakeDecoder` (images CPU numérotées), `FakeInterpolator` (enregistre les couples (a, b, t) demandés), `FakeUpscaler` (plus proche voisin), `FakeEncoder` (paquets = numéros d'image), `FakeMuxer` (journal en mémoire), `FakeStageFactory` (VRAM libre simulable, AV1 absent simulable), horloge injectable. On teste en CI : ordre et nombre d'images, cadence 24/25/30/40/50/60 i/s et VFR, coupures de plan, back-pressure, pause/reprise/annulation à tout moment, propagation d'erreurs, VRAM insuffisante, ETA, règle MP4/MKV, absence de fuite (compteur d'objets vivants dans les pools).

### 3.3 Chaîne d'étages et logique pure

- `Pipeline` construit la chaîne `[Interpolation?] → [SR?] → [HDR?] → [Vers NVENC]` à partir de `OutputPlan` : une `std::vector<std::unique_ptr<IFrameProcessor>>` exécutée dans T2, dans un même flux CUDA (ordre garanti sans événement). Ajouter HDR10 = insérer un étage ; changer l'ordre interpolation/SR = permuter deux éléments.
- `CadencePlanner` (pur) : à partir des `pts` réels de la source (VFR compris), produit pour chaque image de sortie à `k/60 s` le couple d'images sources encadrantes et la position `t ∈ [0,1]` ; `t` à moins d'un demi-millième de 0 ou 1 → copie de l'image (pas d'inférence) ; coupure de plan entre a et b → duplication de l'image la plus proche (critère ENH-012). Durée conservée à ±1 image. Pas de pulldown.
- Détection de coupure : noyau CUDA sur une vignette 160×90 de luminance (histogramme + SAD), seuil réglé sur les clips QA ; calcul dans T1, résultat porté par `FrameFlags::SceneCut`.
- Source ≥ 3840×2160 (ou déjà ≥ la cible) : pas de SR, message (ENH-009) ; source ≥ 50 i/s : pas d'interpolation, horodatages conservés (VFR conservé).

### 3.4 Threads, flux CUDA, files bornées, back-pressure

| Thread | Travail | Flux CUDA | File de sortie (capacité **[H]**) |
|---|---|---|---|
| T0 Démux | `av_read_frame`, aiguillage vidéo / autres pistes | — | paquets vidéo : 64 ; paquets copiés : 256 |
| T1 Décodage | `avcodec_send_packet/receive_frame`, copie GPU→GPU dans le pool source, conversion pivot, rotation, désentrelacement, vignette de coupure | `S_dec` | images source : **8** (= pool source) |
| T2 Traitement | chaîne d'étages | `S_proc` | images 4K : **4** (= pool 4K, surfaces pré-enregistrées NVENC) |
| T3 Encodage | `nvEncEncodePicture` asynchrone, attente des événements de fin, `nvEncLockBitstream` | flux d'E/S NVENC | paquets vidéo : 64 |
| T4 Mux | entrelacement, écriture disque | — | — |
| T5 Rapporteur | agrège compteurs atomiques, calcule i/s et ETA, appelle l'observateur | — | — |

- **Back-pressure** : chaque image occupe une surface d'un **pool de taille fixe** alloué au démarrage du job (aucune allocation dans la boucle chaude). Un étage qui veut une surface et n'en trouve pas attend qu'une surface revienne (destructeur de `PooledSurface`). Les files `BoundedQueue<T>` (mutex + `std::condition_variable_any` + `std::stop_token`) bloquent l'amont quand elles sont pleines. Mémoire bornée quelle que soit la longueur du fichier.
- **Pas de synchronisation CPU dans la boucle** : chaque `VideoFrame` porte un `GpuFence` enregistré sur le flux producteur ; le consommateur fait `cuStreamWaitEvent`. Retour d'une surface au pool : un événement « dernier usage » est enregistré, le pool ne la redonne qu'une fois l'événement passé (`cuEventQuery`, sans blocage). Seul T3 attend la fin d'encodage (événement Win32 de NVENC), ce qui est son rôle.
- **Mémoire épinglée** : seulement pour le repli logiciel (envoi des images décodées par le CPU, `cuMemHostAlloc` + `cuMemcpy2DAsync`, 3 tampons tournants) et pour l'aperçu/avant-après (§8).
- **Pause** : `PauseGate` vérifiée par T0 avant chaque paquet et par T1–T3 entre deux images ; les étages finissent l'image en cours puis attendent. État `Paused` publié quand tous les étages sont arrêtés (< 2 s, en pratique quelques dizaines de ms **[H]**).
- **Annulation** : `std::stop_source` partagé ; toutes les attentes (files, pools, pause) sont interruptibles par `std::stop_token` ; les étages sortent sans vidanger l'encodeur ; le muxer fait `abort()` (fermeture du fichier, **sans suppression** : ADR-049) ; destruction de tous les objets RAII dans l'ordre inverse ; `cuCtxSynchronize` une seule fois avant la libération ; puis `onFinished(Cancelled)`. Cible < 3 s (ENH-021). « Rétablir » (5 s) est géré par l'application (ADR-004) : elle appelle `cancel()` tout de suite (VRAM libérée), supprime le temporaire à l'expiration du délai, et « Rétablir » relance le job depuis le début.
- **Erreur dans un étage** : l'étage publie son `PipelineError` puis déclenche le `stop_source` ; la première erreur gagne, les suivantes (souvent des conséquences) vont au journal seulement.
- **Un seul job à la fois** (budget VRAM) : le moteur refuse un second `start()` (`PipelineErrorCode::Busy`). Les sessions NVENC grand public sont limitées (8 simultanées, à vérifier) : sans effet ici.

### 3.5 Encapsulation RAII des API C

Modèle commun : `template <class T, auto Deleter> using Handle = std::unique_ptr<T, FnDeleter<Deleter>>` pour les pointeurs, et une classe `UniqueValue<T, Deleter>` (déplaçable, non copiable) pour les handles non pointeurs.

| Ressource | Création → libération | Classe |
|---|---|---|
| Contexte CUDA primaire | `cuDevicePrimaryCtxRetain` → `cuDevicePrimaryCtxRelease` | `CudaContext` |
| Flux, événements | `cuStreamCreateWithPriority` / `cuEventCreate(CU_EVENT_DISABLE_TIMING)` → `Destroy` | `CudaStream`, `CudaEvent` |
| Mémoire GPU / épinglée | `cuMemAllocPitch` → `cuMemFree` ; `cuMemHostAlloc` → `cuMemFreeHost` | `DeviceBuffer`, `PinnedBuffer`, `SurfacePool` |
| FFmpeg | `avformat_open_input` → `avformat_close_input` ; `avformat_alloc_output_context2` + `avio_open` → `avio_closep` + `avformat_free_context` ; `avcodec_alloc_context3` → `avcodec_free_context` ; `av_packet_alloc`/`av_frame_alloc` → `_free` ; `av_hwdevice_ctx_create` → `av_buffer_unref` | `AvInput`, `AvOutput`, `AvCodecCtx`, `AvPacket`, `AvFrame`, `AvBuffer` |
| NVENC | `nvEncOpenEncodeSessionEx` → `nvEncDestroyEncoder` ; `nvEncRegisterResource` → `nvEncUnregisterResource` ; `nvEncCreateBitstreamBuffer` → `nvEncDestroyBitstreamBuffer` ; événements Win32 `CreateEventW` → `CloseHandle` | `NvencSession`, `NvencRegistration`, `NvencBitstream`, `WinEvent` |
| TensorRT 10 | `createInferRuntime` / `deserializeCudaEngine` / `createExecutionContext` → `delete` (interface virtuelle) | `std::unique_ptr` direct |
| RTX Video SDK | `NVSDK_NGX_CUDA_Init` + `GetCapabilityParameters` → `NVSDK_NGX_CUDA_DestroyParameters` + `NVSDK_NGX_CUDA_Shutdown` ; `NGX_CUDA_CREATE_VSR` / `NGX_CUDA_CREATE_TRUEHDR` → `NVSDK_NGX_CUDA_ReleaseFeature` (`nvsdk_ngx.h`, `nvsdk_ngx_helpers_vsr.h`). **Le tout dans une DLL pont sans `cudart`** (bloquant B4 de `spike.md` : `nvsdk_ngx_s.lib` compilée en CUDA 11.8, incompatible avec `cudaDeviceProp` de CUDA 13) | `NgxRuntime`, `RtxVideoEffect` (dans `enh_rtxvideo.dll`) |
| FRUC | `NvOFFRUCCreate` → `NvOFFRUCDestroy` ; `NvOFFRUCRegisterResource` → `NvOFFRUCUnregisterResource` (`NvOFFRUC.h`) ; DLL chargée par `LoadLibraryExW` depuis le dossier de l'app | `FrucSession` |
| DLL | `LoadLibraryExW` → `FreeLibrary` | `DynamicLibrary` |

Ordre de destruction garanti par l'ordre de déclaration des membres (inverse de la construction) ; le contexte CUDA est déclaré en premier, donc détruit en dernier.

---

## 4. Préréglages et réglages d'encodage [H]

Les préréglages sont des **données** (fichier versionné, ENH-018, géré par le developer) converties par une fonction pure de l'application en `PipelineParams` typé (§ 6.2). Valeurs initiales proposées, toutes **[H]** :

| | Qualité max | Équilibré (défaut) | Rapide |
|---|---|---|---|
| Super-résolution (niveau `NVSDK_NGX_VSR_QualityLevel`, 0 bicubique … 4 Ultra) | 4 Ultra | 4 Ultra (coût marginal : 3,0 ms contre 2,3 ms en 3 High **[M]**) | 2 Medium |
| Interpolation (< 50 i/s → 60) | RIFE, pleine qualité | RIFE, pleine qualité | dégradée (FRUC ou RIFE à flux demi-résolution) — seul préréglage autorisé à dégrader (décision PO n° 5) |
| NVENC préréglage / réglage | P7, HQ (UHQ si disponible) | P5, HQ | P3, HQ |
| Contrôle de débit | VBR, multipasse pleine résolution, anticipation 32 | VBR, multipasse quart de résolution, anticipation 16 | VBR, une passe, anticipation 0 |
| Débit cible 4K60 HEVC / AV1 (kbit/s) | 60 000 / 45 000 | 40 000 / 30 000 | 25 000 / 19 000 |
| Profondeur par défaut | 10 bits (Main10 / AV1 Main 10 bits) ; 8 bits proposé pour compatibilité | idem | idem |
| Images/s de sortie visées (720p30 → 4K60) | ≥ 30 | ≥ 60 | ≥ 100 |

Débits ajustés au prorata des pixels et de la cadence si la sortie n'est pas 3840×2160 à 60 i/s (ex. source ≥ 50 i/s non interpolée à 50 i/s, sortie 2880×2160).

---

## 5. Budget VRAM et vitesse par étage [H]

Contexte machine : 12 227 Mio au total, ~2 052 Mio pris au repos → ~10 175 Mio libres. Budget job : **≤ 9 216 Mio** (vision §5). Mesure = pic − repos (`cuMemGetInfo` + DXGI `QueryVideoMemoryInfo`, cf. `spike.md` R4).

### 5.1 VRAM par étage, chaîne nominale 720p30 → 4K60 (interpolation avant SR)

| Étage | Contenu | Budget **[H]** (Mio) | Critère de story |
|---|---|---|---|
| Contexte CUDA + modules + cuBLAS/cuDNN éventuels de TensorRT | | 400 | — |
| Décodage NVDEC | surfaces FFmpeg (~20 × 1,4–2,8 Mio en 720p) + état NVDEC ; jusqu'à 1080p 10 bits | 600 | ENH-008 ≤ 600 |
| Pool source (pivot FP16) | 8 × 720p RGBA16F (7 Mio) = 56 ; 1080p : 8 × 16 = 128 | 256 | — |
| Interpolation RIFE TensorRT 720p | poids + activations + espace de travail | 1 024 | ENH-012 |
| Super-résolution RTX Video SDK 720p → 4K | ressources internes du SDK + tampons d'E/S | 2 560 → **mesuré [M] : 188 (effet q3/q4) + 48 (tampons)** | ENH-009 ≤ 3 072 |
| Pool 4K vers NVENC | 4 × P010 4K (12,4 Mio) + 2 × pivot 4K (jusqu'à 66 Mio en FP16) | 256 | — |
| Encodeur NVENC 4K 10 bits | références, anticipation 32, B-frames, tampons de sortie | 1 536 → **mesuré [M] : Équilibré HEVC 869 / AV1 1 266 ; P7 HQ 1 265 / 1 458 ; UHQ 2 094 / 2 262** | ENH-010/011 ≤ 1 536 (UHQ dépasse : critère à revoir si UHQ est retenu) |
| Réserve : SDR→HDR10 (plus tard) | | 512 | ENH-032 |
| Marge de fragmentation / pics transitoires | | 512 | — |
| **Total** | | **7 656** | ≤ 9 216 → marge **1 560** |

**Mesuré en phase 2 [M]** (`spike.md` § P2.3) : chaîne Équilibré complète (FRUC 720p + VSR q3 + conversions + NVENC HEVC, hors décodage) = **1 293 Mio** de pic (+ ~140 Mio de contexte CUDA) ; en AV1 : 1 690 Mio. Le budget de 7 656 Mio est très large ; à réviser à la baisse quand NVDEC et RIFE seront mesurés.

Variante « interpolation après SR » : RIFE à 4K = 2 048–4 096 Mio **[H]** au lieu de 1 024, + pool 4K agrandi → **8 700–10 700 Mio**, hors budget dans le pire cas : argument supplémentaire pour l'ordre retenu. Mesuré avec FRUC **[M]** : variante « après SR » = 1 984 Mio (FRUC 4K = 735 Mio, 22,7 ms/image) et FRUC y répète toutes les images ; l'ordre « avant SR » est confirmé côté VRAM et vitesse (la qualité reste à mesurer).

Contrôle avant démarrage : somme des `vramEstimate()` des étages + marge comparée à la VRAM libre (`cuMemGetInfo`) ; si insuffisante → `PipelineErrorCode::OutOfVram` avec Mio requis / disponibles (ENH-014), pas de plantage. Une allocation qui échoue en cours de route (`CUDA_ERROR_OUT_OF_MEMORY`, `NV_ENC_ERR_OUT_OF_MEMORY`) donne la même erreur, le job finit proprement.

### 5.2 Vitesse par étage, RTX 5070 [H]

| Étage | Débit visé **[H]** | Moteur |
|---|---|---|
| Décodage NVDEC 720p H.264/HEVC | ≥ 600 i/s (critère ENH-008 : ≥ 300) | NVDEC (1 moteur sur la 5070, à vérifier) |
| Conversions couleur + coupure de plan 720p | ≥ 2 000 i/s | SM |
| RIFE 720p FP16 | 250–400 images générées/s | SM / Tensor cores |
| SR 720p → 4K | 70–120 i/s **[H]** → **mesuré [M] : q3 421 i/s (2,3 ms), q4 322 i/s (3,0 ms) ; 10 bits identique** | SM / Tensor cores |
| Conversion pivot → P010 4K | ≥ 1 000 i/s | SM |
| NVENC HEVC 4K60 10 bits | P5 ≈ 100–130 i/s, P7 ≈ 50–70 i/s **[H]** → **mesuré [M] : P3 265 ; Équilibré (P5 HQ, multipasse ¼, anticipation 16) 103 ; P7 HQ multipasse pleine 37 ; P7 UHQ 29** | NVENC (**1 seul moteur, confirmé** par `AppEncCuda -ec`) |
| NVENC AV1 4K60 10 bits | P5 ≈ 80–120 i/s, P7 ≈ 45–60 i/s **[H]** → **mesuré [M] : P3 178 ; Équilibré 105 ; P7 HQ 52 ; P7 UHQ 38** | NVENC |

Débit de bout en bout ≈ min(NVENC, 1 / (temps SM par image de sortie)), où le temps SM par image de sortie = `t_SR + (fraction générée) × t_RIFE + conversions`. Exemple **[H]** Équilibré 30→60 : 10 ms + 0,5 × 3 ms + 0,3 ms ≈ 11,8 ms → ~85 i/s côté SM, plafonné par NVENC P5 ~110 → **~85 i/s**. Si la SR coûte 16 ms, on tombe à ~57 i/s : objectif Équilibré manqué → décision PO n° 5 (qualité d'abord, vitesse affichée).

**Phase 2 [M]** : temps SM par image de sortie (FRUC avant SR, VSR q3–q4) ≈ 3,9–4,6 ms, soit 220–260 i/s côté SM ; **chaîne Équilibré mesurée à 104–118 i/s, plafonnée par l'unique NVENC**. Qualité max ≈ 35 (HEVC) / 50 (AV1) i/s ; Rapide ≈ 170–200 i/s. RIFE (non mesuré) dispose donc d'une marge SM d'environ 10 ms par image générée avant de devenir limitant en Équilibré. Détails : `spike.md` § P2.3–P2.4.

---

## 6. Contrat avec l'application (API publique `src/pipeline/include/enh/pipeline/`)

Le developer n'inclut **que** ces en-têtes (cible `enh_pipeline_api`) et `enh/expected.hpp` (cible `enh_common`). Aucun type CUDA, FFmpeg, Qt ou NVIDIA n'y apparaît : l'interface compile et se teste sans GPU. Le gpu-engineer reste propriétaire de ces en-têtes ; toute évolution passe par une revue commune (developer + gpu-engineer).

### 6.0 Écarts v0.1 ↔ `docs/architecture.md` § 12, tranchés

| # | Écart | Décision | ADR |
|---|---|---|---|
| 1 | `enh::Result<T>` (pipeline) vs `enh::Expected` (application) | **`enh::Expected<T, PipelineError>`** partout dans l'API ; alias unique dans `enh_common` | ADR-003, ADR-047 |
| 2 | Progression ≤ 4/s vs ≤ 10 Hz | **4 fois/s (250 ms)**, plafond de 10 Hz respecté ; étapes, états et fin émis immédiatement ; ETA calculée par le cœur | ADR-051 |
| 3 | `.enhance-part` géré par le pipeline vs ADR-010 | **L'application possède nommage, temporaire, renommage, suppression, nettoyage** ; le pipeline crée en `CREATE_NEW` le chemin exact reçu, écrit, vide, ferme | ADR-010, ADR-049 |
| 4 | `probe`/`plan` dans le moteur vs `IMediaProbe` + `plan` pur | `IMediaProbe::probe` / `thumbnail` CPU seulement (< 1 s) ; **`plan()` fonction libre pure** (`MediaInfo`, `PipelineParams`, `GpuCapabilities`) | ADR-051 |
| 5 | `unique_ptr<IJob>` + `shared_ptr<IJobObserver>` vs `JobHandle` RAII | **`JobHandle`** valeur déplaçable ; observateur par **référence** ; rappels `noexcept`, un seul thread du pipeline, sérialisés, < 1 ms, **aucun appel en retour** | ADR-051 |
| 6 | `ErrorCode` technique vs P-08 | **`PipelineErrorCode` calqué sur P-08** (une ligne = un code) + codes hors P-08 rangés explicitement | ADR-047, § 6.6 |
| 7 | Chemins longs / Unicode vers FFmpeg | **`AVIOContext` Win32 maison** (`CreateFileW` + `\\?\`), pas de chemin UTF-8 donné à FFmpeg ; ouvertures secondaires refusées | ADR-049 |
| 8 | Numérotation P01–P11 | **ADR-040 à ADR-050**, + ADR-051 (contrat) | — |

Noms retenus (ceux de l'application) : `IMediaProbe`, `IPipeline`, `PipelineError`, `PipelineParams`, `JobSpec`, `JobHandle`, `IJobObserver`, `FakePipeline`. Les noms v0.1 `IPipelineEngine`, `PresetSpec`, `IJob`, `Error`, `createEngine`, `createFakeEngine` disparaissent.

### 6.1 Types communs

```cpp
// src/common/include/enh/expected.hpp  (cible enh_common, en-têtes seuls, partagée app + pipeline)
#include <tl/expected.hpp>
namespace enh {
template <class T, class E> using Expected = tl::expected<T, E>;   // std::expected le jour du C++23
template <class E> using Unexpected = tl::unexpected<E>;
}

// src/pipeline/include/enh/pipeline/types.hpp
namespace enh::pipeline {
struct Size { std::uint32_t width = 0, height = 0; };                  // pixels
struct Rational { std::int32_t num = 0, den = 1; };                    // ex. 30000/1001
enum class Container : std::uint8_t { Mp4, Mkv };
enum class TrackKind : std::uint8_t { Audio, Subtitle, Attachment, Data };
}
```

### 6.2 Paramètres et description d'un job (sérialisables)

```cpp
namespace enh::pipeline {
enum class VideoCodec : std::uint8_t { Hevc, Av1 };
enum class BitDepth : std::uint8_t { Bits8 = 8, Bits10 = 10 };
enum class ContainerPolicy : std::uint8_t { Mp4WithMkvFallback, ForceMkv };   // ForceMkv : options avancées (ENH-035)
enum class SuperResQuality : std::uint8_t { Medium, High, Highest };          // correspondance SDK interne
enum class InterpolationQuality : std::uint8_t { Full, Reduced };             // Reduced : « Rapide » seulement (PO n° 5)
enum class DynamicRange : std::uint8_t { Sdr, Hdr10 };                        // Hdr10 : ENH-032, refusé au MVP
enum class RateControl : std::uint8_t { Vbr, ConstQuality };

struct EncodeSettings {
  VideoCodec codec = VideoCodec::Hevc;
  BitDepth depth = BitDepth::Bits10;
  RateControl rate_control = RateControl::Vbr;
  std::uint32_t target_kbps = 40'000;     // pour 3840×2160 à 60 i/s, ajusté au prorata (§ 4)
  std::uint32_t max_kbps = 60'000;
  std::uint8_t nvenc_preset = 5;          // 1..7
  std::uint8_t lookahead_frames = 16;
  bool multipass = true;
};

// Produit par l'application à partir d'un préréglage (fonction pure de l'ADR-005).
struct PipelineParams {
  SuperResQuality super_res = SuperResQuality::High;
  InterpolationQuality interpolation = InterpolationQuality::Full;
  bool interpolate_below_50fps = true;    // décision PO n° 1 : < 50 i/s → 60 i/s, jamais au-delà
  Size target_box_px{3840, 2160};         // boîte englobante, rapport d'aspect conservé (ENH-035)
  DynamicRange range = DynamicRange::Sdr;
  EncodeSettings encode;
  ContainerPolicy container = ContainerPolicy::Mp4WithMkvFallback;
};

struct TimeRange { std::chrono::duration<double> start, duration; };   // secondes

struct JobSpec {
  std::uint32_t schema_version = 1;
  std::filesystem::path input;            // source
  std::filesystem::path output_path;      // chemin EXACT à créer (le temporaire choisi par l'application, ADR-010)
  Container container = Container::Mp4;   // recopié de OutputPlan::container (vérifié par start())
  PipelineParams params;
  std::optional<TimeRange> range;         // aperçu de quelques secondes (ENH-036)
};
}
```
La sérialisation JSON appartient à l'application ; tous les champs sont des types simples et `schema_version` permet de relire une file persistée (ENH-022) ou un journal de reprise (ENH-038).

### 6.3 Avant de lancer : sondage, vignette, plan (sans GPU)

```cpp
namespace enh::pipeline {
struct TrackInfo {
  int index; TrackKind kind; std::string codec; std::string language; std::string title;
  bool mp4_compatible;                     // règle de l'ADR-050
};
struct ColorDescription { /* matrice, primaires, transfert, plage — énumérations du pipeline */ };
struct MediaInfo {
  std::chrono::duration<double> duration;  // s
  Size size_px;                            // taille d'AFFICHAGE (rotation et SAR appliqués)
  Size coded_size_px; Rational sample_aspect; int rotation_deg;
  Rational avg_fps; bool variable_fps;
  std::uint64_t frame_count_estimate;
  std::string video_codec; int bit_depth; ColorDescription color; bool interlaced; bool hdr10;
  std::vector<TrackInfo> tracks;           // audio, sous-titres, pièces jointes
};
struct Thumbnail { Size size_px; std::vector<std::byte> rgba; };   // RGBA 8 bits non prémultiplié, 4 × largeur octets par ligne

class IMediaProbe {                        // CPU seulement ; const, appelable depuis plusieurs threads à la fois
 public:
  virtual ~IMediaProbe() = default;
  virtual Expected<MediaInfo, PipelineError> probe(const std::filesystem::path& input,
                                                   std::stop_token st = {}) const noexcept = 0;   // < 1 s
  virtual Expected<Thumbnail, PipelineError> thumbnail(const std::filesystem::path& input, Size max_px,
                                                       std::stop_token st = {}) const noexcept = 0;
};

enum class PlanNote : std::uint8_t {
  NoUpscaleAlreadyAtTarget, NoInterpolationAtOrAbove50, SoftwareDecode, MkvFallback,
  SubtitleConvertedToMovText, SourceHdr10Kept, Deinterlaced };
struct OutputPlan {
  Size output_px; Rational output_fps; bool upscale; bool interpolate; bool hw_decode;
  Container container; std::string_view extension;   // ".mp4" | ".mkv" (l'application construit les noms)
  std::vector<int> mkv_reason_tracks;                 // pistes qui imposent le MKV (message avant traitement)
  std::uint64_t frames_out_estimate;
  std::uint64_t size_estimate_bytes;                  // prévérification d'espace disque (ENH-024)
  std::uint32_t vram_estimate_mib;                    // à comparer à GpuCapabilities::vram_free_mib
  std::chrono::seconds duration_estimate;             // [H] tables de vitesse mesurées (QA), indicatif
  std::vector<PlanNote> notes;
};

// Fonction PURE : ni E/S, ni GPU, ni état global ; mêmes entrées → même sortie (testée en CI).
Expected<OutputPlan, PipelineError> plan(const MediaInfo& media, const PipelineParams& params,
                                         const GpuCapabilities& caps) noexcept;

std::unique_ptr<IMediaProbe> createMediaProbe();     // implémentation FFmpeg (CPU)
}
```
Règles de `plan()` : interpolation si `avg_fps < 50` (VFR : cadence moyenne) → 60 i/s constants, sinon cadence et horodatages conservés ; super-résolution seulement si le facteur `min(boîte.l / l, boîte.h / h)` est > 1 (sinon `NoUpscaleAlreadyAtTarget`, taille conservée) ; conteneur par l'ADR-050 ; `hw_decode` d'après `caps` (codecs NVDEC) ; erreurs possibles : `UnsupportedFormat`, `ComponentMissing` (ex. AV1 absent), `InvalidParams` (ex. HDR10 au MVP, `Reduced` hors « Rapide » détecté par l'application en amont).
`probe` : limites `probesize` / `analyzeduration` réglées et rappel d'interruption FFmpeg (`AVIOInterruptCB`) avec échéance de 2 s et `stop_token` → jamais bloqué sur un fichier piégé ; `thumbnail` : décodage logiciel d'une image clé vers 10 % de la durée, mise à l'échelle `libswscale` (LGPL), rotation appliquée.

### 6.4 Lancer et piloter un job

```cpp
namespace enh::pipeline {
struct GpuCapabilities {
  std::string gpu_name; std::string driver_version; int compute_major = 0, compute_minor = 0;
  std::uint32_t vram_total_mib = 0, vram_free_mib = 0;
  bool nvenc_hevc_10bit = false, nvenc_av1 = false, nvenc_av1_10bit = false;
  std::vector<std::string> nvdec_codecs;     // codecs décodables par NVDEC
  bool rtx_video_sdk = false, tensorrt = false, interpolation_model = false;
};

enum class JobStage : std::uint8_t { Opening, PreparingModels, Processing, Finalizing };
enum class JobRunState : std::uint8_t { Running, Paused };

class IJobObserver {               // implémenté par l'application ; DOIT survivre au JobHandle
 public:
  virtual ~IJobObserver() = default;
  virtual void onStageChanged(JobStage stage) noexcept = 0;
  virtual void onStateChanged(JobRunState state) noexcept = 0;      // Paused quand effectif (< 2 s)
  virtual void onProgress(const Progress& p) noexcept = 0;          // 4 fois/s
  virtual void onFinished(Expected<JobResult, PipelineError> r) noexcept = 0;  // exactement une fois, en dernier
};

class JobHandle {                  // déplaçable, non copiable
 public:
  JobHandle() noexcept = default;
  JobHandle(JobHandle&&) noexcept; JobHandle& operator=(JobHandle&&) noexcept;
  ~JobHandle();                    // job encore actif → cancel() + attente (≤ 3 s) ; après onFinished → immédiat
  void pause() noexcept;           // non bloquants, idempotents, appelables depuis n'importe quel thread
  void resume() noexcept;          //   (sauf depuis un rappel de l'observateur, voir § 6.7)
  void cancel() noexcept;          // → onFinished(PipelineError{Cancelled}) (< 3 s, VRAM libérée, fichier fermé)
  void wait() noexcept;            // bloque jusqu'au retour de onFinished (tests, fermeture de l'application)
  [[nodiscard]] bool valid() const noexcept;
 private:
  std::unique_ptr<detail::IJobControl> impl_;   // réel ou faux
};

class IPipeline {
 public:
  virtual ~IPipeline() = default;
  virtual Expected<GpuCapabilities, PipelineError> capabilities() noexcept = 0;  // < 1 s, hors thread UI (ENH-011, ENH-031)
  virtual Expected<JobHandle, PipelineError> start(const JobSpec& spec, IJobObserver& obs) noexcept = 0;  // < 50 ms
};

Expected<std::unique_ptr<IPipeline>, PipelineError> createNvidiaPipeline(const PipelineOptions& opt) noexcept;
}
// Pour les tests (cible enh_pipeline_fake) : FakePipeline, FakeMediaProbe (§ 6.8).
```
`start()` ne fait que valider (`Busy`, `InvalidParams`, cohérence `container` / plan) et lancer les threads ; ouverture des fichiers, contrôle de VRAM, construction des modèles TensorRT (`PreparingModels`, 1–3 min au premier lancement **[H]**) se font ensuite et leurs erreurs arrivent par `onFinished`. Si `start()` renvoie une erreur, aucun rappel n'est émis. Un seul job actif par `IPipeline`.

Étapes vues par l'interface (*Lecture · Agrandissement · Fluidité · Enregistrement*, P-05) : traduites **dans le cœur** (fonction pure) à partir de `JobStage` et de l'`OutputPlan` : `Opening` → Lecture ; `Processing` → Agrandissement si `upscale`, sinon Fluidité si `interpolate`, sinon Enregistrement ; `Finalizing` → Enregistrement ; `PreparingModels` → libellé à fournir par l'ux-designer. Les étages tournent en parallèle : une étape « dominante » est plus honnête qu'une fausse séquence.

### 6.5 Progression et résultat

```cpp
namespace enh::pipeline {
struct StageRates { double decode_fps, interpolate_fps, upscale_fps, encode_fps; };   // journal, QA
struct Progress {
  std::uint64_t frames_done;                // images de SORTIE écrites
  std::uint64_t frames_total;               // estimation (durée × cadence de sortie), affinée en cours de route
  double fps;                               // images de sortie / s sur la dernière période (≈ instantané) ; le lissage et l'ETA sont faits par le cœur (EtaEstimator)
  std::chrono::duration<double> elapsed;    // depuis start(), pauses exclues
  std::uint32_t vram_used_mib;              // VRAM du job (cuMemGetInfo, base mesurée avant le job)
  StageRates stage_fps;
};
enum class JobWarning : std::uint8_t { SoftwareDecodeFallback, CorruptFramesConcealed, SubtitleConvertedToMovText, TrackSkipped };
struct JobResult {
  std::uint64_t frames_out; Size output_px; Rational output_fps; Container container;
  std::uint64_t bytes_written;
  std::chrono::duration<double> elapsed; double average_fps; std::uint32_t vram_peak_mib;
  std::vector<JobWarning> warnings;         // aussi au journal ; aucune interruption pour un avertissement
};
}
```

### 6.6 Erreurs (alignées sur P-08)

```cpp
namespace enh::pipeline {
enum class PipelineErrorCode : std::uint16_t {
  // Une ligne du tableau P-08 = un code
  SourceCorrupt,      // « endommagé à partir de 12 min 40 s »     → at
  UnsupportedFormat,  // « format vidéo non pris en charge »        (pas de piste vidéo, codec, 4:4:4 12 bits…)
  OutOfVram,          // « pas assez de mémoire »                  → required_mib, available_mib
  DiskFull,           // « le disque D: est plein » (pendant)       (« avant » = prévérification de l'application)
  DeviceLost,         // « la carte graphique ne répond plus »      (CUDA_ERROR_LAUNCH_FAILED, NV_ENC_ERR_DEVICE_NOT_EXIST, TDR…)
  WriteDenied,        // « ne peut pas écrire dans ce dossier »
  SourceMissing,      // « le fichier d'origine n'est plus là »
  Internal,           // « un problème inattendu »                  + Copier le détail / Afficher les journaux
  // Hors P-08
  Cancelled,          // pas un échec : état « Annulé »
  SourceUnreadable,   // fichier présent mais illisible (verrouillé, droits) : ligne à ajouter à P-08 (ux-designer)
  ComponentMissing,   // → component ; bandeau des prérequis (ENH-031), pas une erreur de carte
  OutputExists,       // l'application choisit un autre nom et relance (jamais affiché)
  InvalidParams,      // défaut de validation côté application → affiché comme Internal
  Busy,               // un job déjà actif → défaut d'orchestration → affiché comme Internal
};
enum class Component : std::uint8_t { None, NvidiaGpu, Driver, NvencHevc, NvencAv1, Nvenc10Bit, RtxVideoSdk, TensorRt, InterpolationModel };

struct PipelineError {
  PipelineErrorCode code = PipelineErrorCode::Internal;
  std::optional<std::chrono::duration<double>> at;       // SourceCorrupt : instant dans la source (s)
  std::uint32_t required_mib = 0, available_mib = 0;     // OutOfVram
  Component component = Component::None;                 // ComponentMissing
  std::string detail;                                    // anglais, technique : journal et « Copier le détail »
  std::int64_t native_code = 0;                          // CUresult, NVENCSTATUS, AVERROR, GetLastError…
};
}
```
Le pipeline ne produit **aucune phrase** ; la présentation traduit le code et ses données en français (ADR-003, ADR-008). Un fichier corrompu **isolé** (quelques images illisibles) n'est pas une erreur : images masquées, `CorruptFramesConcealed` dans `JobResult::warnings` ; `SourceCorrupt` est réservé à un flux qui ne peut plus être lu (seuil à fixer avec QA/secdevops dans ENH-015).

### 6.7 Règles de threads et de durée de vie (fermes)

1. Tous les rappels de `IJobObserver` viennent d'**un seul thread du pipeline** (rapporteur T5), jamais d'un thread d'étage ni du thread UI ; ils sont **sérialisés** ; ordre garanti : `onStageChanged(Opening)` … `onFinished` (dernier, exactement une fois, y compris après annulation ou erreur).
2. Rappels **`noexcept`** et **non bloquants** (< 1 ms) : côté application, copie de la donnée + `IExecutor::post` (ADR-002) ; pas d'E/S, pas de verrou disputé.
3. **Aucun appel en retour** vers le pipeline depuis un rappel (`pause`, `resume`, `cancel`, `wait`, destruction du `JobHandle`, `start`) : l'application reposte d'abord vers son thread. Destruction et `wait()` depuis un rappel = interblocage → assertion en debug.
4. L'observateur doit vivre plus longtemps que le `JobHandle` ; le destructeur du `JobHandle` attend que le dernier rappel soit rendu.
5. `pause`, `resume`, `cancel` sont thread-safe et non bloquants ; leurs effets arrivent par `onStateChanged(Paused | Running)` et `onFinished`.
6. Détruire le `JobHandle` après `onFinished` est immédiat (< 10 ms) ; avant `onFinished`, il annule et attend (≤ 3 s) — à réserver à la fermeture de l'application, hors thread UI si possible.
7. `IMediaProbe` est sans état partagé : plusieurs `probe`/`thumbnail` en parallèle sont permis (pool de l'application, ADR-002) ; interrompables par `stop_token`.
8. Le journal passe par la façade `enh::log` (ADR-007) ; `av_log` est redirigé une seule fois (`av_log_set_callback`) à la création de la sonde ou du pipeline.

### 6.8 Faux pour les tests de l'application (CI, sans GPU)

`FakePipeline` (cible `enh_pipeline_fake`) : scénario déterministe configurable — vitesse (i/s), nombre d'images, étapes, durée de `PreparingModels`, erreur injectée à l'image N (n'importe quel `PipelineErrorCode`), délai de pause / d'annulation, capacités (AV1 absent, VRAM libre) ; horloge **pilotée par le test** (pas de `sleep`) ; un mode « même thread » où les rappels sont émis par un appel `step()` du test. `FakeMediaProbe` : table chemin → `MediaInfo` / erreur, vignette unie. Ces faux respectent les mêmes règles que le réel (§ 6.7) et sont vérifiés par la même suite de tests de contrat (`tests/unit/pipeline/contract/`), exécutée aussi contre le vrai pipeline dans `tests/gpu/`.

---

## 7. Reprise après plantage (prévue, hors MVP — ENH-038)

Conditions posées dès le MVP pour ne rien réécrire :
- `JobSpec` sérialisable avec `schema_version` ; numérotation déterministe des images de sortie (`CadencePlanner` pur) ;
- l'encodeur sait forcer une IDR (`NV_ENC_PIC_FLAG_FORCEIDR`, `nvEncodeAPI.h` l. 680) à une image donnée ;
- le démuxeur sait chercher (`seek`) ; `TimeRange` existe déjà (aperçu).
Mécanique prévue : segments fermés de ~10 s (IDR au début de chaque segment), écrits en MKV/MP4 fragmenté dans un dossier temporaire du job + journal `resume.json` (dernier segment complet, image de sortie, empreinte des réglages). Au redémarrage : on reprend au segment suivant (recherche à l'image clé précédente, décodage jusqu'à l'image exacte, une image source de plus pour l'interpolation), puis remux final sans réencodage en MP4 `faststart`. La même mécanique permettra une pause qui libère la VRAM (ADR-046).

## 8. Extensibilité (vision long terme)

| Besoin | Point d'extension prévu |
|---|---|
| SDR → HDR10 (ENH-032) | étage `IHdrConverter` = **TrueHDR du RTX Video SDK** (interface CUDA, entrée RGB SDR, sortie ABGR10 ou FP16, pilote ≥ r570 ; mesuré 737 i/s et 164 Mio en 4K **[M]**) inséré après la SR ; sortie P010 BT.2020/PQ ; métadonnées HDR10 (mastering display, MaxCLL/MaxFALL) écrites dans le flux NVENC (`outputMasteringDisplay` / `outputMaxCll` en HEVC et en AV1, structures `MASTERING_DISPLAY_INFO` / `CONTENT_LIGHT_LEVEL` : `nvEncodeAPI.h` l. 1711-1728, 1972-1973, 2095-2096) **et** dans le conteneur (`mdcv`/`clli` en MP4, via les données annexes de `AVCodecParameters`) ; `DynamicRange::Hdr10` refusé au MVP (`UnsupportedOption`). Sources déjà HDR10 : au MVP, traitement en 10 bits avec conservation des métadonnées, à valider en QA (risque R7). |
| Aperçu de quelques secondes (ENH-036) | `JobSpec::range` : recherche, décodage à vide jusqu'au début, arrêt à la fin ; même pipeline |
| Avant / après (ENH-033) | `IFrameTap` optionnel sur la sortie du décodage et de la SR : copie asynchrone d'une image réduite vers un tampon épinglé, sur un flux dédié de basse priorité, au plus 1 image/s ; aucune synchro dans la boucle |
| Options avancées (ENH-035) | champs déjà présents dans `PipelineParams` (taille cible, débit, conteneur) ; validés par `plan()` (erreurs typées) |
| Préréglages personnalisés (ENH-034) | `PipelineParams` est une donnée ; identifiant et nom du préréglage restent côté application |
| Dossiers entiers (ENH-037) | côté application (N `JobSpec`) ; rien dans le pipeline |
| Autres cartes RTX 50 | `GpuCapabilities` (nombre de NVENC, AV1, VRAM) ; budget VRAM relatif à la VRAM libre ; encodage « split-frame » si ≥ 2 NVENC (`NV_ENC_SPLIT_ENCODE_MODE`, `nvEncodeAPI.h` l. 2232 ; sans objet sur la 5070, qui n'a qu'un NVENC) |

## 9. Risques

| # | Risque | Effet | Parade |
|---|---|---|---|
| R1 | Le RTX Video SDK n'expose pas d'interop CUDA (seulement D3D11/D3D12) ou n'a pas de mode « hors ligne » | Copie ou interop supplémentaire | **Levé (phase 2)** : interface CUDA native, pas d'interop D3D. **Risque lié découvert (B4)** : `nvsdk_ngx_s.lib` (compilée en CUDA 11.8) plante avec le `cudart` de CUDA 13 → isolement dans une DLL pont sans `cudart`, validé ; risque résiduel si NVIDIA change la lib. Autre point : le « NGX Update Module » du pilote peut télécharger des DLL (avis secdevops) |
| R2 | Format d'entrée/sortie du SDK limité à 8 bits | Perte du 10 bits | **Levé (phase 2)** : ABGR10 accepté en entrée et en sortie, même vitesse qu'en 8 bits |
| R3 | SR plus lente que 60 i/s sur la 5070 avec l'interpolation avant SR (60 images à agrandir) | Équilibré < 60 i/s | Formule ADR-044 + mesure des deux ordres ; décision PO n° 5 |
| R4 | Licence des poids RIFE refusée | Pas de RIFE | FRUC (qualité moindre, à mesurer) ; autre réseau sous licence claire |
| R5 | Un seul NVENC sur la 5070 (**confirmé**) | NVENC est le goulot de toute la chaîne : Équilibré ≈ 105 i/s, P7 HQ 37 (HEVC) / 52 (AV1), UHQ 29 / 38 **[M]** | Qualité max visée à ≥ 30 i/s seulement ; réglages P5 pour Équilibré |
| R6 | Charge GPU à 100 % → animations Qt Quick saccadées (ENH-020 exige 60 i/s d'interface) | Exigence design | Flux CUDA de priorité basse, ordonnancement GPU matériel Windows ; option de limitation (cédant entre deux images) ; mesure QA dès le premier pipeline complet |
| R7 | Sources entrelacées, HDR, 4:2:2/4:4:4, 12 bits | Rendu faux | Désentrelacement : `bwdif_cuda` de libavfilter si présent dans le build LGPL (à vérifier), sinon noyau CUDA maison ; 4:2:2/4:4:4 → décodage logiciel + conversion ; clips QA dédiés |
| R8 | Fichiers malformés faisant planter FFmpeg | Plantage du processus | `Expected` partout, ouvertures secondaires refusées (ADR-049), limites de taille (dimensions, nombre de pistes), fuzzing ENH-015 ; à terme, probe dans un processus séparé (à discuter avec secdevops) |
| R9 | `std::expected` absent en C++20 | Incohérence d'API | **Résolu** : alias unique `enh::Expected` (ADR-003 / ADR-047), `tl::expected` en C++20 |
| R10 | Construction du moteur TensorRT au premier lancement (minutes) | Premier job lent | Étape `BuildingModels` visible, cache local, invalidation sur changement de pilote |
| R11 | Mesure de VRAM par processus `N/A` en WDDM | Budgets mal vérifiés | `cuMemGetInfo` + DXGI dans le processus (cf. `spike.md` R4) |

## 10. À vérifier en phase 2 (fichiers à lire et citer)

**État au 2026-10-21 (fin de phase 2)** : levés, avec fichiers cités dans `spike.md` § P2.2 : NVENC (UHQ, HDR10 HEVC/AV1, `nvEncSetIOCudaStreams`, AV1 10 bits, 1 seul NVENC), RTX Video SDK (CUDA, formats RGB 8/10 bits, niveaux 0–4, TrueHDR, redistribution des DLL `rel`), FRUC (Blackwell via JIT PTX, NV12/ARGB 8 bits, instant arbitraire). **Restent ouverts** : `nvcuvid.h`/`cuviddec.h` et `AppDec` (FFmpeg absent), TensorRT/RIFE (pas d'ONNX), toute la liste FFmpeg ci-dessous, et le « NGX Update Module » (réseau, secdevops).

- Video Codec SDK : `Interface/nvEncodeAPI.h` (préréglages/UHQ, HDR10 HEVC/AV1, `nvEncSetIOCudaStreams`, mode asynchrone, capacités AV1 10 bits, nombre de NVENC), `Interface/nvcuvid.h` / `cuviddec.h` (formats de sortie), exemples `AppEncCuda`, `AppDec`.
- RTX Video SDK : en-têtes et guide (interop CUDA ou D3D, formats d'E/S, niveaux de qualité, SDR→HDR, mode hors ligne, redistribution des DLL).
- Optical Flow SDK : en-tête FRUC (pas temporel arbitraire ?, Blackwell, formats).
- TensorRT 10 : `NvInferRuntime.h` (gestion mémoire, `setDeviceMemory` pour mutualiser l'espace de travail), DLL à redistribuer.
- FFmpeg 8.x (build BtbN `lgpl-shared`) : `libavutil/hwcontext_cuda.h` (`AV_CUDA_USE_PRIMARY_CONTEXT`), présence de `bwdif_cuda`, `avformat_query_codec` pour MP4, écriture `av1C`/`mdcv`/`clli` en MP4 ; `libavformat/movenc.c` (réouverture du fichier de sortie par `io_open` pour `faststart`, ADR-049) ; `libavformat/avformat.h` (`io_open`/`io_close2`, `AVFMT_FLAG_CUSTOM_IO`) ; `libavutil/wchar_filename.h` (chemins longs du protocole `file:`, pour mémoire).
