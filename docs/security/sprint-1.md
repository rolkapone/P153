# Sécurité — Sprint 1

## ENH-008 `[GPU]` — Démuxage FFmpeg et décodage NVDEC (2026-10-23)

Branche `feature/ENH-008-demux-nvdec`, diff `main...feature/ENH-008-demux-nvdec` (6 commits, 60 fichiers) + fichiers QA non commités. Revue APPROVE (cycle 2/3), QA PASS.

**Verdict : PASS** (aucun CRITICAL/HIGH, aucun secret, aucun appel réseau, licences OK).

### Scans

| Contrôle | Résultat |
|---|---|
| `check-no-network.ps1 -SourcesOnly` | OK ; mais exigences positives « non évaluées » (mauvais dossier, SEC-24) → vérifiées à la main ci-dessous |
| `check-no-network.ps1 -Path build/dev` (C1–C4, 252 binaires) | OK ; imports réseau uniquement dans les DLL FFmpeg BtbN (exceptions dev-only E3/E3c/E3d/E3e, échéance 2027-01-21) et Qt6Core (E2) ; exécutables de test `enh_gpu_decode_tests` / `enh_ffmpeg_unit_tests` : imports = FFmpeg + CRT + KERNEL32 seulement |
| ADR-048 (pas de `cuda.lib`) | OK : aucun import `nvcuda.dll` ; `cudart_static` ; `cuCtxGetCurrent` via `cudaGetDriverEntryPointByVersion` ; `nvcuda.dll`/`nvcuvid.dll` préchargées avec `LOAD_LIBRARY_SEARCH_SYSTEM32` |
| gitleaks 8.30.1 (`.gitleaks.toml`) | 6 commits de la branche + `docs/qa/resultats`, `tests/gpu`, `docs/backlog.md`, `docs/sprints/sprint-1.md` : aucune fuite |
| Auteurs / committers | 6/6 = noreply du PO |
| E-mails / chemins personnels | aucun dans le diff ni les rapports QA ; inventaire de logiciels du PO dans le rapport QA (SEC-25, LOW) |
| Fichiers interdits (`git ls-files`) | aucun ; `tests/clips/` ignoré ; corpus généré localement par script, sans téléchargement |
| semgrep / cppcheck / osv-scanner | non installés localement (`pip install semgrep`, `winget install Cppcheck.Cppcheck`, `winget install Google.OSVScanner`) → revue manuelle ciblée à la place ; MSVC `/W4` + clang-tidy faits par la revue |

### Revue manuelle

- **Zéro réseau** : le chemin n'est jamais donné à FFmpeg (URL `""`) ; AVIO maison sur `CreateFileW` en `\\?\` avec `GetFileType == FILE_TYPE_DISK` (refus des tubes et périphériques) ; `io_open` remplacé (refus `EPERM`, compteur, URL non journalisée) ; `protocol_whitelist=file` ; `AVFMT_FLAG_CUSTOM_IO`. `avio_open` n'apparaît que dans `tests/unit/pipeline/ffmpeg/clip_writer.cpp` (écriture d'un clip de test local). Aucun en-tête ni appel réseau.
- **Contextes imbriqués** : confirmé que `concat` (et par construction hls/dash/imf/vobsub) ouvre ses fichiers sans notre `io_open` ; la parade (sondage puis liste blanche avant `avformat_open_input`, `format` imposé ensuite) est correcte et testée (`list.ffconcat` refusé, `DemuxerRefused`). Ajout au threat model §3.1.
- **Liste blanche des démuxeurs** : validée telle quelle.
  - `mov`, `matroska`, `mpegts`, `avi`, `mpeg`, `asf`, `flv`, `ivf` : conteneurs courants ; `mov` n'ouvre les `dref` externes que via `io_open` (refusé) et `enable_drefs` est désactivé par défaut.
  - Flux bruts `h264`, `hevc`, `obu`, `m4v`, `mpegvideo` : le sondage est permissif (un fichier abîmé peut être pris pour un flux brut), mais ces démuxeurs sont minuscules et réutilisent les analyseurs déjà exposés par `mpegts` ; pas de surface nouvelle notable, pas d'ouverture externe. Conséquence fonctionnelle (horodatages et cadence supposés) à couvrir par la QA, pas par la sécurité. **Gardés.**
  - `mxf` : historique de CVE plus fourni que les autres et formats « pro » (DNxHD, AVC-Intra, essence externe non ouverte par FFmpeg) ; **gardé sous condition** : n'avoir que des codecs de la liste blanche derrière (SEC-23) et un fichier MXF dans le corpus de fuzzing ENH-015 ; sinon le retirer de la build de livraison (SEC-06). Confiance moyenne (sources FFmpeg non relues sur ce point).
  - Correspondance par jeton exact (`mpeg` ≠ `mpegts`, `image2pipe` refusé), testée.
- **Robustesse / SEC-13** : `checkedMul`/`checkedAdd`/`alignUp` en `size_t`, dimensions 1..8192 vérifiées sur l'en-tête, après `find_stream_info` et sur **chaque image** (changement de résolution en cours de flux → nouveau pool) ; `av_image_check_size2` ; pas de ligne et plans calculés en `size_t`, conversions `int` bornées (`INT_MAX`) avant `av_image_copy_plane`/`sws_scale` ; seek borné ; exceptions arrêtées aux rappels C (`noexcept`). Reste pour ENH-015 : minimum 16 px, pixels ≤ 8192×4320, `max_streams`, `probesize`, `analyzeduration`, `av_max_alloc`, échéance de sondage, `max_pixels` (SEC-23).
- **Durées de vie** : `fmt_` fermé avant `io_`, tampon AVIO courant libéré (le sondage peut le réallouer), propriété cédée à `avformat_open_input` en cas d'échec ; surfaces GPU rendues au pool par RAII avec événements CUDA. Rien à signaler.
- **Journaux** : pont `av_log` au niveau WARNING, FFmpeg ne connaît pas le chemin ; noms de démuxeurs et codes seulement.
- **DLL** : FFmpeg chargé par import implicite depuis le dossier de l'exécutable ; si le préchargement de `nvcuvid.dll` échoue (ignoré en silence), le chargeur de FFmpeg le cherchera par nom selon l'ordre par défaut : couvert par SEC-08 (ENH-004, en cours).

### Licences

- FFmpeg BtbN : `--enable-version3` → **LGPL-3.0-or-later**, ni `gpl` ni `nonfree` ; dev seulement (inchangé). Registre corrigé ; `deps/ffmpeg.lock.json` à corriger (LOW).
- **dav1d** (repli AV1 logiciel) : BSD-2-Clause, consigné dans `licences.md` §3 ; doit figurer dans le FFmpeg vcpkg de livraison (`ffmpeg[dav1d]`).

### Findings

| Sévérité | Source | Emplacement | Problème | Correction | Quand |
|---|---|---|---|---|---|
| MEDIUM | manuel | `scripts/check-no-network.ps1` l. 139–145 | SEC-24 : exigences positives cherchées dans `src/pipeline/gpu/ffmpeg` (inexistant) → jamais évaluées | devops : cibler le module qui appelle `avformat_open_input`, exiger `protocol_whitelist`, `io_open`, `isAllowedDemuxer`, échouer sinon | avant ou avec le merge (recommandé), non bloquant |
| MEDIUM | manuel | `ffmpeg_decoder.cpp` `create`, `ffmpeg_demuxer.cpp` `open` | SEC-23 : pas de liste blanche de décodeurs (`Other` → n'importe quel décodeur), pas de `format_whitelist`, pas de `max_pixels` | gpu-engineer, ENH-015 | après merge |
| MEDIUM | manuel | limites §3.1 | SEC-13 partiel (voir ci-dessus) | ENH-015 | après merge |
| LOW | manuel | `docs/qa/resultats/2026-10-23_ENH-008.md` l. 4 | SEC-25 : liste des logiciels ouverts sur la machine du PO | qa : formulation générique | avant le commit du rapport |
| LOW | manuel | `deps/ffmpeg.lock.json` | `license` = LGPL-2.1-or-later alors que la build est `version3` | devops : `LGPL-3.0-or-later` | après merge |
| LOW | manuel | `cuda_raii.cpp` `preloadNvidiaDriverDlls` | échec du préchargement de `nvcuvid.dll` ignoré sans journal | journaliser (Info) ; le durcissement réel vient de SEC-08 | ENH-004 / ENH-015 |


## ENH-004 `[UI]` — Squelette Qt Quick et prototype Mica (2026-10-24)

Worktree `F:\P153-wt\ENH-004`, branche `feature/ENH-004-squelette-qt`, diff `main...feature/ENH-004-squelette-qt` (11 commits, 76 fichiers). Revue APPROVE (cycle 2/3 + complément), contrôle design CONFORME, QA PASS (vérifications PO faites à l'écran).

**Verdict : PASS** (aucun CRITICAL/HIGH, aucun secret, aucun e-mail, aucun appel réseau, licences OK).

### Scans

| Contrôle | Résultat |
|---|---|
| `check-no-network.ps1 -Path build/dev` (C1–C4, 279 binaires) | **Échec initial** sur C4 (`enhance.exe`) : 4 URL, toutes des identifiants d'espace de noms XML (`http://schemas.microsoft.com/SMI/2005/WindowsSettings`, `.../SMI/2016/WindowsSettings` ×2 du manifeste, `http://www.w3.org/2000/svg` de l'icône). Script C4 corrigé (SEC-26) et exceptions exactes U1–U3 ajoutées → **OK**. Test négatif : URL ajoutée à une copie de `enhance.exe` → échec C4 comme attendu |
| Imports | `enhance.exe` : Qt6Gui/Quick/Qml/Core, `dwmapi`, `ole32`, `USER32`, `SHELL32`, `KERNEL32`, `GDI32` ; aucun `Qt6Network*`, ni `ws2_32`/`winhttp`/`wininet`/`urlmon` ; Qt6Core : E2 (3 fonctions `ws2_32`) + `mpr`/`netapi32` déjà acceptés |
| Dossier déployé | plugins `qwindows`, `qsvgicon`, `qsvg` seulement ; modules QML `QtQml`, `QtQuick` (+ Effects, Shapes, Templates, Models, WorkerScript) ; aucun `qmltooling`, `tls`, `networkinformation` |
| PE | `DEPENDENTLOADFLAG 0x0A00`, ASLR haute entropie, NX, CFG ; manifeste embarqué unique : `asInvoker`, `uiAccess=false`, `longPathAware`, PerMonitorV2 |
| `enhance --check-modules` | code 0 ; 0 module hors des dossiers de confiance (un module injecté par un logiciel de sécurité installé, signalé en note comme prévu) |
| gitleaks 8.30.1 (`.gitleaks.toml`) | 11 commits de la branche, `scripts/`, rapport QA et CSV non commités : aucune fuite |
| Auteurs / committers | 11/11 = noreply du PO ; aucune mention de l'outil d'IA dans les messages ; config git locale **et globale** = noreply (SEC-05 clos) |
| E-mails / chemins personnels | aucun e-mail dans le diff ; seul chemin : `F:\P153-wt\ENH-004` dans `docs/reviews/ENH-004.md` (chemin de projet, déjà d'usage dans la doc) ; rapport QA : un utilitaire personnel cité (SEC-29) |
| Captures QA (`docs/qa/resultats/ENH-004/captures/*.png`, 5 fichiers) | regardées une à une : fenêtre d'Enhance seule, aucune autre fenêtre ni élément du bureau ; aucun bloc de métadonnées texte dans les PNG |
| Fichiers interdits (`git ls-files`, diff) | aucun (`.agents/`, `.codex/`, `AGENTS.md` présents dans `F:\P153` restent non suivis : ne jamais les ajouter) |
| semgrep / cppcheck / osv-scanner | non installés (`pip install semgrep`, `winget install Cppcheck.Cppcheck`, `winget install Google.OSVScanner`) → revue manuelle ciblée ; aucune dépendance vcpkg ajoutée par la branche |

### Revue manuelle

- **SEC-08** confirmé (détail au threat model §4) : `SetDefaultDllDirectories` + `SetDllDirectoryW(L"")` en tête de `main`, `/DEPENDENTLOADFLAG:0xA00`, `qt.conf` embarqué (`Prefix=.`), `setLibraryPaths` avant l'application, variables Qt effacées, imports QML limités aux ressources + `<app>/qml`, `--check-modules` (codes 3/5) testé. Reste `SetSearchPathMode` (SEC-28, LOW).
- **SEC-20** confirmé : manifeste lu dans le binaire, bandeau si jeton élevé. Clos.
- **Cache de pipeline Qt** (`%LOCALAPPDATA%\Enhance\cache\qtpipelinecache-*`) : écrit et relu au démarrage, consigné au threat model §3.12 (SEC-27, LOW, risque R-3).
- **Registre** : lecture seule de 3 valeurs DWORD sous HKCU (`AppsUseLightTheme`, `EnableTransparency`, `DWM\ColorPrevalence`) par `RegGetValueW` typé ; rien à signaler.
- **Shader `glass.frag`** : aucune entrée externe, pas de boucle, divisions protégées.
- **Options de ligne de commande** : liste fermée (`--glass-testbed`, `--no-mica`, `--check-modules`, `--glass-quality=0|1|2`), valeurs inconnues ignorées ; aucun chemin accepté.
- **Capture pour la bascule de thème** (`grabToImage`) : en mémoire seulement, aucun fichier écrit.
- **Tests Qt** : variables `QT_PLUGIN_PATH` / `QML_IMPORT_PATH` posées par CTest pour les tests seulement (l'exe les efface).

### Licences

Registre mis à jour : modules Qt réellement déployés (LGPL-3.0, avis BSD-3/HPND de qtsvg), Qt Shader Tools (outil de build, exception GPL de Qt pour le `.qsb` produit), icône provisoire (création de l'équipe), polices système Cascadia Mono / Consolas non livrées. Rien de nouveau à livrer hors Qt.

### Findings

| Sévérité | Source | Emplacement | Problème | Correction | Quand |
|---|---|---|---|---|---|
| MEDIUM (corrigé) | outil + manuel | `scripts/check-no-network.ps1` C4 | SEC-26 : première occurrence seulement, exceptions inopérantes | corrigé par secdevops dans le worktree (à commiter par devops avec `network-allowlist.json`) | avec le merge |
| LOW | manuel | `%LOCALAPPDATA%\Enhance\cache\qtpipelinecache-*` | SEC-27 : cache Qt relu au démarrage | doc utilisateur + désinstalleur ; désactiver si élévation | ENH-025 / ENH-030 |
| LOW | manuel | `process_security.cpp` `hardenDllSearchPath` | SEC-28 : `SetSearchPathMode` absent | ajouter l'appel + test | ENH-016 au plus tard |
| LOW | manuel | `docs/qa/resultats/2026-10-23_ENH-004.md` l. 75 | SEC-29 : utilitaire personnel cité | formulation générique | avant le commit du rapport |
| INFO | manuel | branche ENH-004 | branche partie avant `c083fb8` : `threat-model.md` et `licences.md` du worktree ont été repris de `main` puis complétés (fusion vérifiée sans conflit par `git merge-file`) ; cette section n'est pas posée dans le worktree (le fichier `sprint-1.md` n'existe que sur `main` : conflit d'ajout garanti) | devops : après le merge, ajouter cette section à la fin de `docs/security/sprint-1.md` sur `main` (commit `docs(sec):`) | au merge |
