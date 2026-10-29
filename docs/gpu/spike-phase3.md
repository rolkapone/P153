# Spike — Phase 3 (P3.1 à P3.8)

> Suite de [spike.md](spike.md) — extrait du fichier d'origine lors du ménage docs du 2026-10-29, texte inchangé.

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
