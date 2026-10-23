# Sprint 1 — « Une fenêtre, un décodeur, une super-résolution »

Statut : **DÉMARRÉ le 2026-10-21, validé par le PO avec les choix par défaut** (état au 2026-10-23 : ENH-008 DONE ; ENH-004 en contrôle design ; ENH-040 IN PROGRESS ; ENH-047 TODO). Rédigé le 2026-10-21 par le scrum-master.

### Décisions du PO (2026-10-21)
1. Barre d'état « GeForce RTX 5070 · prête » **en dur** dans le squelette (détection réelle plus tard, ENH-031).
2. Capsule de démonstration « Tester le ressort » **visible** à la démo.
3. Démo : **fenêtre calme d'abord** ; `--glass-testbed` (motif sous le verre) seulement sur demande.
4. Icône d'application **provisoire** (`docs/design/ui/icones/enhance-app-provisoire.svg`) acceptée pour ENH-004 ; icône définitive plus tard.

### Arbitrages techniques (orchestrateur)
- Critère UX n° 4 d'ENH-004 lu comme **« libellé >= 4,5:1 et anneau de focus >= 3:1 »** (le bord de la capsule n'est pas porteur d'information, WCAG 1.4.11).
- Mesures de coût du verre **pendant un vrai traitement GPU** (design-system §12 n° 7 à 9) **reportées à ENH-010** ; ENH-004 mesure au repos et sous charge GPU artificielle.
- Compléter le catalogue `maquettes/index.html` avec le bouton de verre autonome au prochain passage UI.
Capacité : 15 pts (vitesse par défaut, aucun historique). Engagé : **15 pts**.
Plan de release et jalons : `docs/backlog.md` (J1 = fin de ce sprint).

## Objectif du sprint

Montrer au PO la **fenêtre Enhance** (sombre, fond Mica, aux couleurs de la maquette validée) et, en parallèle, la **première brique réelle du pipeline** sur la RTX 5070 : un fichier vidéo décodé sur NVDEC à plus de 1 000 images/s et une image 720p agrandie en 4K par la super-résolution RTX Video, mesurées par `enhance-bench`.

## Stories retenues

| Story | Titre | Tags | Pts | Qui |
|---|---|---|---|---|
| ENH-004 | Squelette de l'application Qt Quick + prototype Mica | [UI] | 5 | developer (+ ux-designer, ui-designer) |
| ENH-008 | Démuxage FFmpeg et décodage NVDEC (matrice « noyau ») | [GPU] | 5 | gpu-engineer |
| ENH-040 | DLL pont RTX Video et chargement différé des DLL NVIDIA | [GPU] | 3 | gpu-engineer |
| ENH-047 | `enhance-bench` : banc en ligne de commande pour la QA (v1) | [GPU] | 2 | gpu-engineer |
| | **Total** | | **15** | |

Pourquoi ces quatre, dans cet ordre :
- **ENH-004** ouvre la voie à toutes les stories `[UI]` et lève tôt le risque A1 (Mica sous Qt Quick + Direct3D 11) ; c'est ce que le PO verra en premier.
- **ENH-008 + ENH-040** sont les deux maillons dont dépendent le plus de stories GPU (conversions, super-résolution, interpolation, encodage). ENH-040 supprime le bloquant B4 (plantage de NGX avec CUDA 13) sous une forme livrable.
- **ENH-047** donne à la QA son premier instrument officiel et rend la démo tangible (une image avant / après).
- Les conversions couleur (ENH-042) et l'encodage (ENH-010) ne tiennent pas dans 15 pts : le premier fichier 4K de bout en bout est le jalon **J2 (Sprint 2)**.
- **Redécoupage d'ENH-008** (estimation gpu-engineer : 8 pts pour la matrice complète) : ENH-008 (5 pts, ici) garde AVIO maison, RAII, hwaccel, repli logiciel, SEC-13 et la matrice « noyau » (8 combinaisons dont ProRes en repli + `BBB-FAST`) ; **ENH-008b (3 pts, hors sprint, avant ENH-048, au plus tard Sprint 3)** porte le reste de la matrice, 8 / 10 bits, VFR, rotation, dimensions impaires. Le sprint reste à 15 pts ; le total MVP passe à 174.
- **Non retenu** : l'export RIFE (ENH-043, 5 pts) est prévu au Sprint 3 ; il passe devant ENH-008 si le PO préfère sécuriser la licence tout de suite (voir `backlog.md`, « Questions pour le PO » n° 1).

## Ordre d'exécution et dépendances

```
developer :    ENH-004 --------------------------------->   (en parallèle, sous réserve des 2 réponses du PO)
gpu-engineer : ENH-008 ---------> ENH-040 ---> ENH-047
                                   (ENH-047 a besoin du décodeur et du pont)
```
**Priorité d'exécution** : 1. ENH-008, 2. ENH-040, 3. ENH-047 (agents GPU en séquence, un seul gpu-engineer) ; ENH-004 en parallèle par le developer. Les étapes reviewer / qa-tester / secdevops / devops de chaque story s'intercalent sans bloquer l'implémentation de la suivante (l'orchestrateur lance les agents indépendants en parallèle). Si le temps manque, l'ordre de sacrifice est : repli ENH-004 (fond opaque) avant tout retard sur 008 / 040.
Une branche par story : `feature/ENH-004-squelette-qt`, `feature/ENH-008-demux-nvdec`, `feature/ENH-040-pont-rtx-video`, `feature/ENH-047-enhance-bench`. Merge dans `main` (`--no-ff`) dans l'ordre 008, 040, 047 ; ENH-004 indépendante. Aucun push (pas de remote).

## Design et notes techniques au planning (à montrer au PO en une seule fois avant de coder)

| Story | Livrable de planning | Auteur | Statut |
|---|---|---|---|
| ENH-004 `[UI]` | UX : `docs/design/ux/ENH-004.md` (périmètre strict du squelette, 9 états, 10 critères testables, 2 questions PO). UI : maquette `docs/design/ui/maquettes/enh-004-squelette.html` + `docs/design/ui/ENH-004-specs.md` (ui-designer, **en cours d'écriture en parallèle**) ; coque de `ecran-principal.html` (validée) | ux-designer, ui-designer | UX fait ; UI en cours |
| ENH-008 `[GPU]` | `docs/gpu/ENH-008.md` : approche (hwaccel CUDA, AVIO maison, repli logiciel), matrice noyau, budgets, **estimation 8 pts -> redécoupage appliqué** (ENH-008 5 pts + ENH-008b 3 pts) | gpu-engineer | Fait |
| ENH-040 `[GPU]` | `docs/gpu/ENH-040.md` : cause de B4, découpage du pont, API NGX, budget (>= 300 i/s, <= 300 Mio), limites | gpu-engineer | Fait |
| ENH-047 `[GPU]` | `docs/gpu/ENH-047.md` : arguments, schéma du JSON, format d'image du mode `vsr-image` | gpu-engineer | Fait |
| Seuils QA | `docs/qa/sprint-1-seuils.md` : décodage 720p **>= 1 000 i/s**, VRAM décodage **<= 600 Mio**, VSR **>= 300 i/s** et **<= 300 Mio**, écart `enhance-bench` / `nvidia-smi` **< 10 %**, 10 fichiers à la suite : dérive **<= 50 Mio** ; checklist UI v1 `docs/qa/checklist-ui.md` pour ENH-004. **Critères UX et seuils QA reportés dans `backlog.md`** (ENH-004, 008, 008b, 040, 047) | qa-tester, scrum-master | Fait |

## Découpage en tâches

### ENH-004 — Squelette Qt Quick + prototype Mica  [UI] — 5 pts (developer) — branche `feature/ENH-004-squelette-qt`
Critères UX : `docs/design/ux/ENH-004.md` §7 (UX-1 à UX-10, repris dans `backlog.md`) ; seuils : `docs/qa/checklist-ui.md`. Design : `maquettes/enh-004-squelette.html`, `ENH-004-specs.md`. Décisions PO reçues (voir en-tête) : plus aucune condition au démarrage.
| # | Tâche | Qui |
|---|---|---|
| T1 | Valider la maquette et les specs ENH-004 (périmètre §1 de la note UX : rien d'autre que le squelette) et la grille de contrôle design | ui-designer, ux-designer |
| T2 | Cible `enhance` (WIN32) + `enh_app` + module QML `Enhance.App` ; intégrer `Theme.qml` / `qmldir` (module `Enhance.Design`) ; `qmlcachegen` | developer |
| T3 | **SEC-08 / SEC-20** : `SetDefaultDllDirectories` + `SetDllDirectoryW(L"")` en première instruction de `wWinMain`, `/DEPENDENTLOADFLAG:0xA00`, `qt.conf` + `setLibraryPaths`, variables Qt ignorées en release, manifeste `asInvoker` + `longPathAware` + PerMonitorV2 ; test des chemins réels des modules chargés | developer |
| T4 | **Prototype Mica** dans `enh_platform` (classe RAII DWM : backdrop, barre de titre sombre) ; mesures à 100 % et 200 % ; **limité à ~1 jour** : sinon repli fond opaque teinté, décision en ADR | developer |
| T5 | `Main.qml` : coque de la maquette, textes via `qsTr`, un `Q_PROPERTY` affiché + une action QML -> C++ ; animation ressort mesurée à 60 i/s | developer |
| T6 | Tests : GoogleTest sur le modèle C++, Qt Quick Test (`offscreen`), contrôle « pas de `Qt6Network.dll` » (C2) | developer |
| T7 | Mesure du démarrage à froid (< 2 s) ; captures 100 / 125 / 150 / 200 %, sombre | developer, qa-tester |
| T8 | Rubrique « À retenir en C++ » (RAII d'une ressource Win32, `std::unique_ptr` avec suppresseur) | developer |
| T9 | Tâche secdevops : lignes du registre (Qt modules, **icônes Lucide ISC**, polices) ; vérification SEC-08 | secdevops |
| T10 | Revue (3 cycles max) -> **contrôle design** (CONFORME / ECARTS, captures 100-200 %, sombre et clair, replis) -> **qa-tester** (checklist-ui, 60 i/s, démarrage < 2 s, UX-1 à UX-10 sauf Narrateur = PO) -> **secdevops** (scan, zéro réseau, licences Qt / Lucide / polices) -> **devops** (build, tests, merge `--no-ff`) -> DONE | reviewer, ui-designer, qa-tester, secdevops, devops |

### ENH-008 — Démuxage FFmpeg et décodage NVDEC (matrice noyau)  [GPU] — 5 pts (gpu-engineer) — branche `feature/ENH-008-demux-nvdec`
Note : `docs/gpu/ENH-008.md` (fait). Seuils : `docs/qa/sprint-1-seuils.md` §3. Matrice étendue = ENH-008b (hors sprint).
| # | Tâche | Qui |
|---|---|---|
| T0 | Déplacer `enh/expected.hpp` vers `enh_pipeline_api` / `enh_common` ; découpage CMake `enh_pipeline_ffmpeg` (sans CUDA) / `enh_pipeline_gpu`, à valider avec devops | gpu-engineer, devops |
| T1 | Interface `IDemuxer` / `IDecoder` / `IFrameSink` et faux (CI) | gpu-engineer |
| T2 | `AVIOContext` maison sur `CreateFileW` (`\\?\`), `io_open` restrictif, `protocol_whitelist=file` ; tests chemin > 260, accents, UNC | gpu-engineer |
| T3 | Classes RAII FFmpeg / CUDA ; décodage NVDEC (hwaccel), surfaces NV12 / P010 en mémoire GPU ; repli logiciel + message au journal | gpu-engineer |
| T4 | **SEC-13** : `checked_mul`, tailles en `std::size_t`, dimensions bornées avant allocation | gpu-engineer |
| T5 | Corpus **noyau** (`tests/gpu/corpus/make-corpus.ps1`, `tests/quality/New-DecodeCorpus.ps1`, rien de commité) : 8 combinaisons de la note §4 + `BBB-FAST.mp4` ; test « images décodées = `ffprobe` » | gpu-engineer, qa-tester |
| T6 | Tests unitaires (sans GPU) + `tests/gpu` ; mesure i/s et VRAM ; 10 fichiers à la suite sans fuite | gpu-engineer, qa-tester |
| T7 | « À retenir en C++ » (RAII d'un `AVFormatContext`, suppresseur de `unique_ptr`) | gpu-engineer |
| T8 | Revue (3 cycles max) -> **qa-tester** (seuils §3 : >= 1 000 i/s, <= 600 Mio, dérive <= 50 Mio, repli, cas limites) -> **secdevops** (SEC-13, `io_open`, licences FFmpeg dev) -> **devops** (build + tests unitaires + `scripts/test-gpu.ps1`, merge `--no-ff`) -> DONE | reviewer, qa-tester, secdevops, devops |

### ENH-008b — Matrice de formats étendue  [GPU] — 3 pts — **hors Sprint 1** (avant ENH-048, au plus tard Sprint 3)
Branche prévue `feature/ENH-008b-matrice-formats`. Tâches à détailler à son planning : corpus étendu, variantes 8 / 10 bits, VFR, rotation, dimensions impaires, changement de résolution en cours de flux ; mêmes étapes de workflow que ENH-008.

### ENH-040 — DLL pont RTX Video  [GPU] — 3 pts (gpu-engineer) — branche `feature/ENH-040-pont-rtx-video`
Note : `docs/gpu/ENH-040.md` (fait). Seuils : `docs/qa/sprint-1-seuils.md` §3 (VSR >= 300 i/s, <= 300 Mio, q4 != q0 >= 30 %, imports sans `cudart`, 10 cycles <= 50 Mio, qualité d'image part Équilibré >= 35 %).
| # | Tâche | Qui |
|---|---|---|
| T1 | Note `docs/gpu/ENH-040.md` ; partir du pont jetable du spike (`build/spike/bench/rtxvideo_bridge.*`) | gpu-engineer |
| T2 | `rtxvideo_bridge.dll` (sans `cudart`), `IUpscaler` + faux upscaler, RAII de l'effet NGX | gpu-engineer |
| T3 | Chargement différé, chemin absolu, DLL `rel` seulement, préchargement System32 des DLL pilote, vérification de signature (SEC-08) | gpu-engineer |
| T4 | Test `tests/gpu` : 720p -> 4K q4 d'une image synthétique != bicubique ; test d'imports (`dumpbin`, aucun `cudart*.dll`) ; 10 cycles création / destruction sans dérive | gpu-engineer |
| T5 | Mesure : >= 300 i/s, <= 300 Mio ; texte de signalement à NVIDIA prêt pour le PO | gpu-engineer |
| T6 | Relecture renforcée (liaison atypique, `cudaDeviceProp` partiel) ; 3 cycles max | reviewer |
| T7 | **qa-tester** (vitesse, VRAM, q4 != q0, `dumpbin`, 10 cycles, qualité d'image via `vsr-image` une fois ENH-047 mergée ; sinon test `tests/gpu`) -> **secdevops** (SEC-08, signature NVIDIA, licences SDK) -> **devops** (build + `test-gpu.ps1`, merge `--no-ff`) -> DONE | qa-tester, secdevops, devops |

### ENH-047 — `enhance-bench` v1  [GPU] — 2 pts (gpu-engineer) — branche `feature/ENH-047-enhance-bench`
Note : `docs/gpu/ENH-047.md` (fait). Seuils : `docs/qa/sprint-1-seuils.md` §3 (JSON complet, répétabilité <= 3 %, écart `nvidia-smi` < 10 %, code de sortie). Dépend d'ENH-008 et ENH-040 mergées.
| # | Tâche | Qui |
|---|---|---|
| T1 | Relire `docs/gpu/ENH-047.md` (arguments, JSON, format d'image) et l'aligner sur les commandes de la QA | gpu-engineer, qa-tester |
| T2 | Cible `enhance-bench` (`ENH_WITH_GPU=ON`) : `--stage decode` + rapport JSON (`frames_*`, `wall_s`, `fps_out`, `stages`, VRAM pic / repos, mémoire privée) | gpu-engineer |
| T3 | `--stage vsr-image --in --out [--q 0..4]` | gpu-engineer |
| T4 | Branchement dans `Measure-Speed.ps1` ; vérification croisée avec `nvidia-smi` (< 10 %) | qa-tester, gpu-engineer |
| T5 | Revue (3 cycles max) -> **qa-tester** -> **secdevops** -> **devops** (merge `--no-ff`) -> DONE | reviewer, qa-tester, secdevops, devops |

### Fin de story (toutes)
Chaque story suit le workflow de `CLAUDE.md` : implémentation -> **reviewer** (3 cycles max) -> ui-designer (`[UI]`) -> **qa-tester** (`[UI]` / `[GPU]`) -> **secdevops** -> **devops** (build + tests unitaires + tests GPU locaux, merge `--no-ff`) -> scrum-master (DONE).

### Tâches transverses hors points (parallèles)
| Qui | Tâche |
|---|---|
| qa-tester | Terminer le rapport `docs/qa/resultats/2026-10-21_reference-broadcast.md` ; mettre à jour `protocole.md` §7.1 (seuils en part de marge, décision n° 14, proposition SSIM / PSNR) ; créer `checklist-ui.md` |
| ux-designer | Corriger `parcours.md` P-06 (VRAM conservée à la pause, décision n° 8) ; aligner la version de pilote de P-01 une fois tranchée |
| secdevops | Ligne « icônes » du registre (Lucide, ISC) ; relire `licences.md` après ENH-004 |
| scrum-master | Mettre à jour le board et le plan de release à chaque merge |

## Board

| Story | Statut (TODO / IN PROGRESS / IN REVIEW / QA / SECURITY / DONE / BLOCKED) | Cycles de review | Notes |
|---|---|---|---|
| ENH-004 Squelette Qt Quick + Mica [UI] | IN REVIEW (contrôle design / corrections mineures) | à confirmer par l'orchestrateur | Branche `feature/ENH-004-squelette-qt` ; décisions PO prises (barre d'état en dur, capsule visible, icône provisoire) ; risque A1 (Mica) ; SEC-08 / SEC-20 inclus ; écarts mineurs en correction avant QA |
| ENH-008 Démuxage + NVDEC, matrice noyau [GPU] | **DONE** (2026-10-23) | 2 / 3 | Merge `0c2f748` (--no-ff) ; revue APPROVE ; QA PASS : 3 398 i/s (BBB-FAST), 2 541 i/s (BBB-PAN), 58 Mio, dérive 0 ; secdevops PASS. Reports : UNC -> ENH-015 ; SEC-23 + fin SEC-13 + troncature -> ENH-015 ; journal `nvcuvid.dll` -> ENH-008b |
| ENH-040 Pont RTX Video [GPU] | **IN PROGRESS** | 0 | Branche `feature/ENH-040-pont-rtxvideo` ; priorité 2 ; relecture renforcée |
| ENH-047 `enhance-bench` v1 [GPU] | TODO | 0 | Priorité 3 ; dépend d'ENH-040 (ENH-008 mergée) |
| *ENH-008b Matrice de formats étendue [GPU]* | *hors sprint* | - | 3 pts, avant ENH-048, au plus tard Sprint 3 ; + petite tâche : journaliser l'échec de préchargement de `nvcuvid.dll` |

Points livrés dans ce sprint : **5 / 15** (ENH-008).

Suites ajoutées au backlog le 2026-10-23 : SEC-23 et fin de SEC-13 + avertissement de fichier tronqué (ENH-015), journal `nvcuvid.dll` (ENH-008b), recoupement PresentMon avant ENH-010.

## Risques du sprint

| # | Risque | Parade |
|---|---|---|
| S1-1 | **Mica** instable avec une fenêtre Qt Quick transparente + Direct3D 11 (risque A1) | Prototype limité à ~1 jour ; repli fond opaque teinté par les tokens, quasi indiscernable ; décision en ADR |
| S1-2 | ENH-008 dépasse 5 pts (AVIO maison + matrice de 30 combinaisons + repli logiciel) | **Traité** : estimation 8 pts confirmée, redécoupage appliqué (ENH-008 5 pts + ENH-008b 3 pts, matrice étendue après le Sprint 1) |
| S1-3 | ENH-004 chargée (SEC-08, SEC-20, Mica, tests QML) | Tâche T3 par défaut, pas de nouvelle fonctionnalité visible au-delà de la coque ; contrôle design sur la coque seule |
| S1-4 | Le pont (ENH-040) repose sur le préfixe 11.x de `cudaDeviceProp` : fragile si le SDK change | Test d'import + test de fumée GPU à chaque changement de SDK ; signalement à NVIDIA |
| S1-5 | Mesures faussées par la VRAM déjà occupée au repos (2 à 3,8 Gio : Docker, iCUE, navigateur) | Mesure en différence, applications graphiques fermées pendant les mesures officielles (QA) |
| S1-6 | FFmpeg BtbN (dev) n'a pas la configuration minimale de livraison | Accepté en développement (exceptions datées 2027-01-21) ; ENH-048 avant ENH-030 |
| S1-8 | ENH-008 : test d'ouverture UNC (`Win32File.PartageUnc`, `ENH_TEST_UNC_DIR`) non exécuté faute de partage accessible | **Décision PO 2026-10-23** : point « non vérifié — reporté à ENH-015 » (critère « test UNC exécuté sur un vrai partage » ajouté à ENH-015) |
| S1-7 | Pas de remote : la CI GitHub n'est pas vérifiée | Tests unitaires et GPU locaux obligatoires ; création du dépôt à discuter au Sprint 2 |

## Définition de la démo de fin de sprint (à faire au PO, ~10 minutes)

1. **La fenêtre** (fenêtre calme d'abord ; `--glass-testbed` seulement sur demande du PO ; capsule « Tester le ressort » visible) : lancer `enhance.exe` (build local, `scripts/build.ps1 -Preset dev`) : fenêtre « Enhance » en moins de 2 s, thème sombre, barre de titre sombre, fond Mica (ou repli teinté), texte français, une animation ressort fluide. Regarder à **100 % puis 200 %** de mise à l'échelle Windows : rendu net, rien de tronqué. Comparer à `maquettes/ecran-principal.html`.
2. **Le décodage** : `enhance-bench --in tests\clips\in720\BBB-FAST.mp4 --stage decode --report decode.json` : nombre d'images = `ffprobe`, **>= 1 000 images/s**, VRAM <= 600 Mio ; ouvrir le JSON.
3. **La super-résolution** : extraire une image du clip 720p, puis `enhance-bench --stage vsr-image --in image720.bmp --out image4k.bmp --q 4` ; ouvrir côte à côte avec la même image agrandie par Lanczos (FFmpeg) : **voir la différence de netteté**. Afficher les i/s et la VRAM du rapport (>= 300 i/s, <= 300 Mio).
4. **Les contrôles** : `scripts\test-gpu.ps1` vert (tests de fumée, pont, décodage, boucle de 10 fichiers sans fuite VRAM) ; aucun `Qt6Network.dll` dans le dossier de sortie.

Critères de réussite du sprint : les 4 stories `DONE`, démo sans intervention du développeur, mesures conformes aux seuils ci-dessus. Non attendus à ce stade : fichier 4K encodé (J2), interpolation (J3), glisser-déposer (ENH-017).

## Revue (à remplir en fin de sprint)

- Livré / non livré :
- Comment faire la démo : voir « Définition de la démo » ci-dessus (à actualiser avec les chemins réels).
- Dernières mesures qualité / vitesse :

## Rétro (à remplir en fin de sprint)

- Ce qui a marché :
- Ce qui a coincé (ex. stories à 3 cycles) :
- Actions pour le sprint suivant (1 à 3) :
