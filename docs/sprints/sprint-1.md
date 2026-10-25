# Sprint 1 — « Une fenêtre, un décodeur, une super-résolution »

Statut : **TERMINÉ le 2026-10-25 : 4 stories sur 4 DONE, 15 / 15 points ; `main` = `76c559e`, poussée sur GitHub.** Démarré le 2026-10-21, validé par le PO avec les choix par défaut. Rédigé le 2026-10-21 par le scrum-master. **On s'arrête ici et on attend le PO avant d'ouvrir le Sprint 2.**

### Décisions du PO (2026-10-21)
1. Barre d'état « GeForce RTX 5070 · prête » **en dur** dans le squelette (détection réelle plus tard, ENH-031).
2. Capsule de démonstration « Tester le ressort » **visible** à la démo.
3. Démo : **fenêtre calme d'abord** ; `--glass-testbed` (motif sous le verre) seulement sur demande.
4. Icône d'application **provisoire** (`docs/design/ui/icones/enhance-app-provisoire.svg`) acceptée pour ENH-004 ; icône définitive plus tard.

### Décisions du PO (2026-10-23, numérotation continue du backlog)
17. La barre de titre suit la couleur d'accentuation Windows choisie par l'utilisateur (`ColorPrevalence`).
18. L'opacité du verre sur fond vif sera réglée à ENH-016 sur du vrai contenu (piste : teinte allégée dans le biseau).
19. Aucun agent n'envoie de vraies frappes clavier ni de clics sur le poste du PO (incident QA d'ENH-004).

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
- **Non retenu** : l'export RIFE (ENH-043, 5 pts) est prévu au Sprint 3 ; composition validée par le PO le 2026-10-21 (voir `backlog.md`, « Résolues (historique) »).

## Ordre d'exécution et dépendances

```
developer :    ENH-004 --------------------------------->   (en parallèle, sous réserve des 2 réponses du PO)
gpu-engineer : ENH-008 ---------> ENH-040 ---> ENH-047
                                   (ENH-047 a besoin du décodeur et du pont)
```
**Priorité d'exécution** : 1. ENH-008, 2. ENH-040, 3. ENH-047 (agents GPU en séquence, un seul gpu-engineer) ; ENH-004 en parallèle par le developer. Les étapes reviewer / qa-tester / secdevops / devops de chaque story s'intercalent sans bloquer l'implémentation de la suivante (l'orchestrateur lance les agents indépendants en parallèle). Si le temps manque, l'ordre de sacrifice est : repli ENH-004 (fond opaque) avant tout retard sur 008 / 040.
Une branche par story : `feature/ENH-004-squelette-qt`, `feature/ENH-008-demux-nvdec`, `feature/ENH-040-pont-rtxvideo`, `feature/ENH-047-enhance-bench`. Merge dans `main` (`--no-ff`) dans l'ordre 008, 040, 047 ; ENH-004 indépendante. Aucun push (pas de remote).

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

### ENH-040 — DLL pont RTX Video  [GPU] — 3 pts (gpu-engineer) — branche `feature/ENH-040-pont-rtxvideo`
Note : `docs/gpu/ENH-040.md` (fait). Seuils : `docs/qa/sprint-1-seuils.md` §3 (VSR >= 300 i/s, <= 300 Mio, q4 != q0 >= 30 %, imports sans `cudart`, 10 cycles <= 50 Mio). Décision PO n° 20 (2026-10-24) : le seuil 300 i/s en q4 8 bits devient un avertissement (10 bits >= 300 reste) et le critère de qualité d'image (part Équilibré >= 35 %) est déplacé dans ENH-009.
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
| ENH-004 Squelette Qt Quick + Mica [UI] | **DONE** (2026-10-23) | 2 / 3 | Merge `79a3e93` (--no-ff) ; revue APPROVE ; contrôle design CONFORME ; QA PASS (vérifications à l'écran par le PO) ; secdevops PASS ; risque A1 (Mica) levé. Suites au backlog : SEC-27, SEC-28, B2, dette design |
| ENH-008 Démuxage + NVDEC, matrice noyau [GPU] | **DONE** (2026-10-23) | 2 / 3 | Merge `0c2f748` (--no-ff) ; revue APPROVE ; QA PASS : 3 398 i/s (BBB-FAST), 2 541 i/s (BBB-PAN), 58 Mio, dérive 0 ; secdevops PASS. Reports : UNC -> ENH-015 ; SEC-23 + fin SEC-13 + troncature -> ENH-015 ; journal `nvcuvid.dll` -> ENH-008b |
| ENH-040 Pont RTX Video [GPU] | **DONE** (2026-10-24) | 2 / 3 | Merge `986d7e3` (--no-ff), poussée `75f27d4`, issue #23 fermée ; revue APPROVE cycle 2 ; QA PASS sur le périmètre du pont (décision n° 20, rapport `docs/qa/resultats/2026-10-24_ENH-040.md`) ; secdevops PASS ; `test-gpu` 32 / 32 après merge. Critère de qualité VSR déplacé dans ENH-009 (après ENH-053) ; 300 i/s en q4 8 bits = avertissement (290 mesuré), 10 bits 305 OK. Suites : SEC-30 à SEC-36 + dette `tidy.ps1` (26 avertissements) -> ENH-041 / ENH-030 |
| ENH-047 `enhance-bench` v1 [GPU] | **DONE** (2026-10-25) | 1 / 3 | Merge `76c559e` (--no-ff), poussée ; revue APPROVE au cycle 1 + corrections ; QA **FAIL** (décodage partiel mesuré comme complet) puis **PASS** après le correctif `373ced9` (le banc refuse de mesurer un décodage partiel) ; secdevops PASS ; `test-gpu` 41 / 41. Baseline `main` : décodage BBB-FAST 4 100 i/s, BBB-PAN 2 870 i/s ; VSR q4 8 bits ~300 i/s, 10 bits ~315 i/s ; VRAM 58 / 198 Mio (décodage) et 188 / 364 Mio (VSR, repos / pic). Suites LOW : SEC-37, SEC-38, SEC-39 (backlog), SEC-40 (-> ENH-030) |
| *ENH-008b Matrice de formats étendue [GPU]* | *hors sprint* | - | 3 pts, avant ENH-048, au plus tard Sprint 3 ; + petite tâche : journaliser l'échec de préchargement de `nvcuvid.dll` |

Points livrés dans ce sprint : **15 / 15** (ENH-008 5, ENH-004 5, ENH-040 3, ENH-047 2).

Décisions du PO du 2026-10-24 (n° 22 à 25, détail dans `backlog.md`) : poids RIFE téléchargés par l'équipe au sprint 3 ; Enhance exclu de WER ; seuils SSIM / PSNR validés ; notification de fin de file au MVP = icône de la barre des tâches + résumé + son (toast avec l'installeur).

**Clips de test** : la copie de 19 Go dans le worktree a été remplacée par une jonction (traité, plus de tâche devops en attente).
Hors sprint, créée le 2026-10-24 : ENH-053 (spike qualité SR, 5 pts, avant ENH-009), voir `docs/backlog.md` « Mouvements v0.5 ».

Suites ajoutées au backlog le 2026-10-23 : SEC-27 (cache de pipeline Qt : ENH-025 / ENH-030), SEC-28 (`SetSearchPathMode`, avant ENH-016), B2 (pics de rendu à rejouer machine calme), dette design (pointillés du cadre depuis un jeton, illustration en .svg), SEC-23 et fin de SEC-13 + avertissement de fichier tronqué (ENH-015), journal `nvcuvid.dll` (ENH-008b), recoupement PresentMon avant ENH-010.

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

## Revue (2026-10-25)

**Vitesse réelle : 15 / 15 points** (4 stories sur 4). Premier sprint terminé : la vitesse de 15 pts / sprint n'est plus une hypothèse mais une mesure (un seul point de donnée).

### Livré
| Story | Pts | Merge | Cycles | Verdicts |
|---|---|---|---|---|
| ENH-008 Démuxage FFmpeg + NVDEC | 5 | `0c2f748` | 2 / 3 | QA PASS, secdevops PASS |
| ENH-004 Squelette Qt Quick + Mica + verre `[UI]` | 5 | `79a3e93` | 2 / 3 | design CONFORME, QA PASS (vérifications à l'écran par le PO), secdevops PASS |
| ENH-040 DLL pont RTX Video | 3 | `986d7e3` | 2 / 3 | QA PASS sur le périmètre du pont (décision n° 20), secdevops PASS |
| ENH-047 `enhance-bench` v1 | 2 | `76c559e` | 1 / 3 + corrections | QA FAIL puis PASS, secdevops PASS |

Autres livrables du sprint : premier push et **CI GitHub verte** après correctifs (security 6 / 6 ; ci : format, sast, core, tidy) -> **ENH-003 DONE** ; backlog GitHub (5 jalons, 54 issues de stories, tableau Kanban « Enhance — Sprints ») ; README « Équipe » et ligne `Agent:` (décision n° 21) ; décisions du PO n° 16 à 25 ; risque A1 (Mica) levé ; blocage B4 (NGX / CUDA 13) levé par le pont.

### Non livré / reporté
- Rien d'engagé n'est resté en dehors du sprint. Critères déplacés ou reportés (aucun perdu) : qualité d'image VSR -> ENH-009 derrière le spike **ENH-053** (décision n° 20) ; test UNC, SEC-23, fin de SEC-13 -> ENH-015 ; journal `nvcuvid.dll` -> ENH-008b ; mesures de coût du verre pendant un vrai traitement -> ENH-010 ; SEC-30 à SEC-36 -> ENH-041 / ENH-030 ; SEC-37 à SEC-39 -> backlog, SEC-40 -> ENH-030.

### Dernières mesures (RTX 5070, pilote 617.42, `main` = `76c559e`)
| Mesure | Valeur | Seuil |
|---|---|---|
| Décodage NVDEC BBB-FAST (720p) | **4 100 i/s** (3 398 mesurés à la QA d'ENH-008) | >= 1 000 |
| Décodage NVDEC BBB-PAN | **2 870 i/s** (2 541 à la QA d'ENH-008) | informatif |
| VSR q4 720p -> 4K, 8 bits | **~300 i/s** (avertissement, décision n° 20) | >= 300 (avertissement) |
| VSR q4 720p -> 4K, 10 bits | **~315 i/s** | >= 300 (bloquant) |
| VRAM décodage (repos / pic) | 58 / 198 Mio | <= 600 Mio |
| VRAM VSR (repos / pic) | 188 / 364 Mio | <= 300 Mio (effet seul) |
| Dérive VRAM, 10 fichiers à la suite | 0 | <= 50 Mio |
| `scripts\test-gpu.ps1` | **41 / 41** | tout vert |
| Qualité VSR q4 vs Lanczos | +13 % de la marge VMAF seulement (BBB-FAST sous Lanczos) | cible Équilibré 35 % -> **spike ENH-053** |

Réserve : ces vitesses varient de **5 à 15 % d'une session à l'autre** selon la charge de la machine du PO (instabilité observée) ; les comparaisons « pas de régression > 5 % » exigent une carte au repos (action n° 1 de la rétro).

### Comment faire la démo au PO (~10 minutes)
Depuis `F:\P153`, sur `main` (`76c559e`), PowerShell. Si le build n'est pas à jour : `scripts\build.ps1 -Preset dev`. Les exécutables sont dans `build\dev` ; le préréglage `local-gpu` donne les mêmes binaires dans `build\local-gpu`. Fermer navigateur, Docker et iCUE avant de regarder les chiffres.

1. **La fenêtre** (fenêtre calme d'abord) :
   ```powershell
   F:\P153\build\dev\src\enhance.exe
   ```
   Attendu : fenêtre « Enhance » en moins de 2 s, thème sombre, barre de titre sombre (couleur d'accentuation Windows si activée), fond Mica, textes français, barre d'état « GeForce RTX 5070 · prête » (en dur), capsule « Tester le ressort » (animation fluide). Regarder à 100 % puis 200 % de mise à l'échelle Windows. Sur demande :
   ```powershell
   F:\P153\build\dev\src\enhance.exe --glass-testbed    # motif sous le verre, pour voir la matière
   F:\P153\build\dev\src\enhance.exe --no-mica          # repli opaque teinté, pour comparer
   ```
2. **Le décodage** :
   ```powershell
   $B = "F:\P153\build\dev\src\tools\bench\enhance-bench.exe"
   & $B --stage decode --in F:\P153\tests\clips\in720\BBB-FAST.mp4 --report $env:TEMP\decode.json
   code $env:TEMP\decode.json
   ```
   Attendu : `frames_in` égal au nombre d'images de `ffprobe`, `fps_out` **>= 1 000** (~4 100 sur ce poste), `vram_stage_mib` <= 600, code de sortie 0.
3. **La super-résolution, avec comparaison à Lanczos** :
   ```powershell
   $FF = "C:\Tools\ffmpeg\bin"; $T = "$env:TEMP\enh-demo"; mkdir $T -Force | Out-Null
   & "$FF\ffmpeg" -y -i F:\P153\tests\clips\in720\BBB-FAST.mp4 -vf "select=eq(n\,100)" -frames:v 1 "$T\image720.bmp"
   & $B --stage vsr-image --in "$T\image720.bmp" --out "$T\image4k_vsr.bmp" --q 4 --report "$T\vsr.json"
   & "$FF\ffmpeg" -y -i "$T\image720.bmp" -vf "scale=3840:2160:flags=lanczos" "$T\image4k_lanczos.bmp"
   ```
   Ouvrir `image4k_vsr.bmp` et `image4k_lanczos.bmp` côte à côte (Photos, zoom 100 % sur un contour ou du texte). Honnêteté vis-à-vis du PO : la différence est **visible mais modeste sur ce clip** (mesure : +13 % de la marge VMAF), c'est la raison du spike ENH-053. `vsr.json` donne les i/s et la VRAM.
4. **Les contrôles** :
   ```powershell
   F:\P153\scripts\test-gpu.ps1                                   # attendu : 41 / 41
   Get-ChildItem F:\P153\build\dev\src -Recurse -Filter Qt6Network.dll   # attendu : aucun résultat
   ```
Non attendu à ce stade : fichier 4K encodé (J2), interpolation (J3), glisser-déposer (ENH-017), détection réelle de la carte (ENH-031).

### Les 5 « À retenir en C++ » du sprint (synthèse des 4 stories)
1. **RAII au lieu de `goto cleanup`** : `std::unique_ptr` avec un suppresseur sur mesure (`AVFormatContext`, `FreeLibrary`, `HANDLE`, `ComPtr`) libère tout à la sortie de portée, même sur un `return` d'erreur ; `OutputFile` du banc va plus loin (RAII « transaction » : le fichier de sortie est supprimé sauf si `commit()` a été appelé).
2. **L'ordre de déclaration des membres est l'ordre de destruction** (inverse) : le contexte FFmpeg est fermé avant l'AVIO qu'il utilise, l'effet NGX avant le runtime puis la DLL. Les classes qui possèdent une ressource sont **déplaçables et non copiables** (`= delete`, `std::exchange`) : un seul propriétaire, pas de double libération ; `std::shared_ptr` seulement quand la durée de vie est vraiment partagée (surface GPU et son pool).
3. **Erreurs par valeur** : `enh::Expected<T, PipelineError>` oblige à tester la réussite avant de lire le résultat et transporte le code natif (AVERROR, `cudaError_t`, `GetLastError`) ; à une frontière de DLL, `extern "C"` + `struct_size` + `try { … } catch (...)` dans chaque export : aucune exception ne traverse la frontière.
4. **Une interface abstraite pour tester sans matériel** : `IVramProbe`, `IUpscaler`, `IDecoder` ont une version réelle et un faux pour la CI sans GPU (en C : une structure de pointeurs de fonctions, sans vérification par le compilateur) ; même idée pour le temps : `std::chrono` avec une horloge passée en paramètre, aucun `sleep` dans les tests.
5. **Des types qui évitent les pièges du C** : `enum class`, `std::optional` (pas de -1 sentinelle), `std::span` / `std::string_view` (pas de `strcmp`), `std::cmp_greater` (comparaison signé / non signé sûre) et `std::jthread` + `stop_token` (le thread de mesure VRAM s'arrête et est joint tout seul).

## Rétro (2026-10-25)

### Ce qui a marché
- **Les 4 stories livrées, aucune à 3 cycles** : 3 stories à 2 cycles, 1 à 1 cycle (+ corrections). Le redécoupage d'ENH-008 au planning (5 + 3 pts) a évité un dépassement.
- **Les mesures ont servi de garde-fou, pas de formalité** : la QA a refusé ENH-047 (décodage partiel mesuré comme complet) et ENH-040 a révélé tôt que la qualité de la VSR était faible (+13 %) -> décision n° 20 et spike ENH-053 *avant* d'investir dans ENH-009.
- **Le risque Mica (A1) levé** en une story ; le design est CONFORME avec vérification à l'écran par le PO.
- **GitHub opérationnel** : CI verte, backlog en issues et Kanban, traçabilité des commits (`Agent:`).
- **Sécurité en continu** : SEC-01 atténué (Job Object contre l'updater NGX), suites LOW triées et rattachées à des stories, rien de CRITICAL / HIGH ouvert.

### Ce qui a coincé
- **Incident QA d'ENH-004** : des frappes clavier de la QA ont été détournées vers le navigateur du PO -> règle n° 19 (plus de vraies frappes ni de clics sur le poste du PO).
- **Trois coupures dues aux limites d'utilisation** en cours de sprint : reprises coûteuses, état à reconstituer depuis le dépôt.
- **Mesures de vitesse instables** (5 à 15 % d'une session à l'autre, machine du PO chargée : VRAM de fond de 2 à 3,8 Gio) : un FAIL ou un PASS peut tenir à la charge de la machine, pas au code.
- **Copie de 19 Go de clips dans un worktree** : lente et fragile ; remplacée par une jonction.
- **Cycles de revue** : 3 stories sur 4 ont eu besoin d'un second cycle ; un défaut de mesure (décodage partiel) n'a été vu qu'à la QA, après la revue.

### Actions pour le Sprint 2 (4)
1. **Carte au repos pour toute mesure officielle** : avant chaque mesure, `nvidia-smi` doit montrer une charge GPU < 5 % et une VRAM de fond notée dans le rapport ; applications graphiques fermées ; baseline `main` rejouée dans la même session que la mesure de la branche. Écart de 5 à 15 % entre sessions = « non concluant », jamais FAIL de régression. *(qa-tester ; devops : ajouter ce contrôle en tête de `Measure-Speed.ps1`)*
2. **Réduire les cycles de revue** : l'implémenteur fait passer `clang-format`, `tidy.ps1` et une auto-relecture sur la liste des motifs apparus aux cycles 1 de ce sprint, et ajoute un test « cas d'échec » à chaque critère de mesure (ex. fichier partiel) avant de demander la revue. *(gpu-engineer, developer ; reviewer fournit la liste en début de sprint)*
3. **Tenir compte des limites d'usage** : tâches de planning courtes et commitables, une étape de workflow par lancement d'agent, état de la story consigné dans `docs/sprints/sprint-2.md` après chaque étape pour reprise sans perte ; pas plus de deux agents lourds en parallèle. *(orchestrateur, scrum-master)*
4. **Worktrees et clips** : un worktree par story, clips partagés par jonction ou `ENH_CLIPS_DIR` (jamais de copie), procédure d'une ligne dans `scripts\README` ; supprimer les worktrees fusionnés. *(devops)*
5. **Ne pas laisser dériver les suites de sécurité** : SEC-30 à SEC-36 sont rassemblées dans ENH-041 (Sprint 2) ; SEC-37 / SEC-38 / SEC-39 (LOW) sont traitées avec la première story qui touche au banc ou à l'écriture de sorties (ENH-010). *(secdevops, scrum-master)*
