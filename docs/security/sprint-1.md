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
- **dav1d** (repli AV1 logiciel) : BSD-2-Clause, consigné dans `licences-composants.md` §3 ; doit figurer dans le FFmpeg vcpkg de livraison (`ffmpeg[dav1d]`).

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

## ENH-040 `[GPU]` — DLL pont RTX Video (2026-10-24)

> Section placée en tête pour que la fusion avec `main` (qui a ajouté ENH-004 en fin de fichier) se fasse sans conflit ; devops peut la déplacer après ENH-004.

Branche `feature/ENH-040-pont-rtxvideo`, diff `main...feature/ENH-040-pont-rtxvideo` (6 commits, 46 fichiers) + rapport QA non suivi `docs/qa/resultats/2026-10-24_ENH-040.md`. Revue APPROVE (cycle 2/3), QA PASS sur le périmètre du pont (décision PO n° 20).

**Verdict : PASS** (aucun CRITICAL/HIGH ouvert pour cette story, aucun secret, aucun e-mail, aucun fichier interdit, aucun appel réseau dans notre code ; licences OK, 1 point à confirmer avant ENH-030).

### Scans

| Contrôle | Résultat |
|---|---|
| `check-no-network.ps1 -SourcesOnly` (branche) | OK (82 fichiers) ; `NVSDK_NGX_UpdateFeature` absent de `src/` |
| `check-no-network.ps1 -Path build/dev` (branche, 262 binaires) | OK |
| Arbre **fusionné** (`git merge-tree --write-tree main feature/ENH-040-pont-rtxvideo` : fusion propre ; script et liste blanche de `main` + modifications de cette story) sur `build/dev` et `build/dev/src` | OK, y compris C4 durci (SEC-26) **avec le pont classé binaire d'Enhance** : aucune URL, aucun chemin personnel ni du dépôt, PDB sans chemin ; seules chaînes de chemin = 4 fichiers sources NVIDIA de `nvsdk_ngx_s.lib` (`C:/dvs/p4/build/sw/devrel/libdev/NGX/...`), barres obliques, non visées par les motifs : classées « code tiers lié », aucune exception requise. Avant correction, le pont n'était **pas** analysé par C4 (motif `*enhance*.dll`, SEC-33, corrigé) |
| Imports du pont (`objdump -p`) | `ADVAPI32`, `USER32`, `KERNEL32` seulement ; aucun import différé ; aucun `cudart` ; `nvcuda.dll` chargée à l'exécution par le chargeur statique de `cuda.lib` (SEC-35) |
| Imports de `enh_pipeline_rtxvideo` (exécutables de test aujourd'hui, `enhance.exe` demain) | `wintrust.dll` (7 fonctions), `crypt32.dll` (1) : **acceptés** (vérification Authenticode hors ligne) ; `wintrust.dll` ajouté aux `warnImports` + entrée `acknowledgedWarnings` (SEC-34) |
| Exports du pont | 7 `rvb_*` + **27 `NVSDK_NGX_*`** (directives de `nvsdk_ngx_s.lib`, dont `NVSDK_NGX_UpdateFeature`) : liste consignée dans SEC-32 |
| Pilote (constat hors dépôt) | `nvngx_update.exe` (DriverStore actif) importe `WS2_32`, `CRYPT32`, `bcrypt` : capable de réseau, d'où l'importance du blocage ; `_nvngx.dll` n'importe aucune DLL réseau (télémétrie via `NGXNVAPITelemetryInit`, passage par NVAPI probable [H]) |
| gitleaks 8.30.1 (`.gitleaks.toml`) | 6 commits de la branche + rapport QA : aucune fuite |
| Auteurs / committers | 6/6 = noreply du PO |
| Messages de commit / diff | aucune attribution d'outil (ni `Co-Authored-By`, ni « Generated with ») |
| E-mails / chemins personnels / nom de machine | aucun dans le diff ni dans le rapport QA (le chemin `%ProgramData%\NVIDIA\NGX\models\…` cité est système, pas personnel) |
| Fichiers interdits / binaires | aucun binaire dans le diff (`git diff --numstat` sans fichier binaire) ; `git ls-files` : aucun `.claude/`, `CLAUDE.md`, `third_party/`, DLL, `.lib`, clip ; la DLL `rel/nvngx_vsr.dll` est seulement copiée au build (`enh_deploy_rtxvideo`), jamais la `dev` (`ngx_static_policy_test`) |
| semgrep / cppcheck / osv-scanner | non installés localement (`pip install semgrep`, `winget install Cppcheck.Cppcheck`, `winget install Google.OSVScanner`) → revue manuelle ciblée ; pas de nouvelle dépendance vcpkg |

### Revue manuelle

- **SEC-01, processus enfants** : `ChildProcessBlocker` (Job Object sans nom, `JOB_OBJECT_LIMIT_ACTIVE_PROCESS = 1`, imbrication comptée sous verrou, limite levée à la destruction) entoure `rvb_runtime_create` **et** `rvb_vsr_create`. Le processus reste rattaché à vie au job (sans `KILL_ON_JOB_CLOSE` : un processus lancé plus tard par l'app survit à sa fermeture, comportement normal). QA : 0 `nvngx_update.exe` sur 15 inits. **Faiblesse** : échec ouvert si le job est indisponible (SEC-30, MEDIUM).
- **SEC-01, DLL de fonction** : vérifiée **avant** (fichier livré signé « NVIDIA Corporation ») et **après** chargement (chemin annoncé par le journal NGX → même fichier par chemin final, ou contenu identique octet pour octet, ou signature NVIDIA hors ligne avec avertissement `VsrFeatureNotShipped` sans chemin ; sinon `RtxVideoFeatureUntrusted`) : conforme à la décision PO du 2026-10-24. Échec fermé si NGX n'annonce plus la DLL (format du journal changé). Limites : détection après exécution (R-7) ; magasin NGX inscriptible par `BUILTIN\Utilisateurs` (constaté, SEC-31) et course entre chargement et comparaison.
- **Authenticode** : `WTD_REVOKE_NONE`, `WTD_REVOCATION_CHECK_NONE`, `WTD_CACHE_ONLY_URL_RETRIEVAL`, `WTD_DISABLE_MD2_MD4`, `WTD_UI_NONE`, état toujours fermé (`WTD_STATEACTION_CLOSE` par RAII) ; signataire comparé au nom simple du certificat feuille d'une chaîne validée par Windows ; fichier non signé → refus (test unitaire). Mise à jour automatique des racines non mesurée (SEC-34, [H]).
- **Journal NGX** : niveau ON (nécessaire), `DisableOtherLoggingSinks = true`, lignes analysées en mémoire, renvoi au journal d'Enhance seulement si demandé ; cible du rappel protégée par verrou lecteurs/rédacteur et oubliée sur tout échec (`CreateRollback`) : pas de pointeur pendant. Test GPU : aucun fichier écrit par NGX, magasin système inchangé.
- **`UpdateFeature` / `AppId`** : aucun appel (C1 + `ngx_static_policy_test` qui interdit aussi `_Init_with_ProjectID`, `ProjectIdDescription`, `x64/dev`, et impose un seul `NVSDK_NGX_CUDA_Init(kApplicationId,` avec `kApplicationId = 0`). `PathListInfo` vide.
- **Frontière C du pont** : `try/catch(...)` sur chaque export, `struct_size` vérifiée, un seul runtime (`compare_exchange`), arrêt refusé tant qu'un effet vit ; `cudart_shim` n'écrit que le préfixe 11.x de `cudaDeviceProp` (408 octets, `static_assert` sur les décalages) — robuste tant que le SDK ne change pas (test GPU + `dumpbin /symbols` à chaque nouvelle version).
- **Chargement de DLL** : pont par chemin absolu (`LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | SYSTEM32`), contrôle d'ABI, `/DEPENDENTLOADFLAG:0x800` ; `nvcuda.dll` chargée par nom par `cuda.lib`, sûre grâce au préchargement System32 (dépendance implicite, SEC-35).
- **Entiers / tailles** : rectangles non nuls vérifiés des deux côtés ; tailles réelles des surfaces garanties par `makeUpscaleRequest` (contrat documenté), le pont ne peut pas les vérifier seul.

### Licences

- `nvsdk_ngx_s.lib` liée statiquement dans le pont : **OK**, RTX SDKs License art. 1.c (« incorporated in object code format into a software application ») ; le CLUF d'Enhance devra couvrir le pont (SEC-21) ; 27 exports : pas une distribution autonome à mon avis, à confirmer par le PO si lecture prudente souhaitée. Registre `licences.md` § 1 et § 2.1 mis à jour.
- CRT statique `/MT` du pont : OK (« Distributable Code » de Visual Studio).
- `cuda.lib` (CUDA 13.4, chargeur statique incorporé au pont) : **non listé** dans l'Attachment A (Windows) de l'EULA CUDA → **à confirmer par le PO avant ENH-030**, ou supprimé par le correctif recommandé (SEC-35). Non bloquant : rien n'est livré à ce stade.
- Signalement NVIDIA (§ 12 d'ENH-040) : non envoyé, facultatif ; ajouter le constat SEC-31.

### Findings

| Sévérité | Source | Emplacement | Problème | Correction | Quand |
|---|---|---|---|---|---|
| MEDIUM | manuel (confiance élevée) | `ngx_guard.cpp`, `rtxvideo_upscaler.cpp` | SEC-30 : Job Object indisponible → init NGX poursuivie avec un avertissement (échec ouvert) | refuser l'effet si `updaterBlocked() == false` (ou décision PO) + test | ENH-041, avant livraison |
| MEDIUM | manuel + `icacls` | `%ProgramData%\NVIDIA\NGX\models` (pilote) | SEC-31 : magasin NGX inscriptible par tous les utilisateurs ; notre contrôle est postérieur au chargement | contrôle préalable du magasin (ENH-041) ; signalement NVIDIA ; risque R-7 | ENH-041 |
| LOW | `objdump -p` | `enh_rtxvideo_bridge.dll` | SEC-32 : 27 exports `NVSDK_NGX_*` ré-exportés, dont `UpdateFeature` | accepté ; figer la liste exacte dans le test GPU | ENH-041 |
| LOW | contrôle C4 | `scripts/network-allowlist.json` | SEC-33 : pont hors du périmètre C4 | **corrigé ici** (`enh_rtxvideo_bridge.dll` ajouté à `ownBinaries`) | fait |
| LOW | manuel (confiance moyenne) | `module_trust.cpp` | SEC-34 : `WinVerifyTrust` hors ligne non encore mesuré pare-feu fermé | M2 en ENH-041 ; imports acceptés | ENH-041 |
| LOW (+ licence) | manuel | `rtxvideo_bridge/CMakeLists.txt` (`CUDA::cuda_driver`) | SEC-35 : `cuda.lib` hors Attachment A ; `nvcuda.dll` chargée par nom | charger `nvcuda.dll` depuis System32 dans le pont (`GetProcAddress`) ou confirmation PO | avant ENH-030 |
| LOW | manuel | `docs/gpu/ENH-040.md` § 4, l. 88 | SEC-36 : note en décalage avec le code (journal OFF, exports `rvb_*` seuls, énumération des modules) | renvoyer au § 10 / ADR-048 am. 1 | ENH-041 |

**SEC-01** : passe de HIGH à **MEDIUM (atténué)**, porte ENH-041 (mesure réseau sur session complète, télémétrie d'init R-8, SEC-30, SEC-31) ; repasse HIGH si `enhance.exe` émet du trafic. Threat model mis à jour (§ 3.6, § 3.8, § 3.8.1, § 3.9, SEC-01, SEC-30 à SEC-36, R-7, R-8, § 7).

## ENH-047 `[GPU]` — `enhance-bench` v1 (2026-10-25)

Branche `feature/ENH-047-enhance-bench`, 7 commits (`44bcc90` → `373ced9`), revue APPROVE, QA PASS (re-QA cycle 2, test-gpu 41/41). Outil de développement **hors livrable**.

**VERDICT : PASS** (aucun CRITICAL/HIGH ; 4 LOW pour le backlog).

### Scans

| Contrôle | Résultat |
|---|---|
| `check-no-network.ps1 -SourcesOnly` | OK (C1 : 142 fichiers) |
| `check-no-network.ps1 -Path build/dev` et `-Path build/local-gpu` | OK. `enhance-bench.exe` (build du 2026-10-24 19:17, après `373ced9`) : **aucun** import réseau (`ws2_32`, `winhttp`, `wininet`, `urlmon` absents) ; `wintrust.dll` (7) + `crypt32.dll` (1) = vérification Authenticode hors ligne d'ENH-040 (SEC-34, déjà acceptée) ; chaînes propres analysées (C4, `ownBinaries` contient bien `enhance-bench.exe`). Les FFmpeg copiés dans `src/tools/bench/` sont la build de **développement** (exceptions `dev-only` existantes E3…, jamais livrées) |
| Code réseau dans le diff (revue) | aucun ; FFmpeg ouvert par le démuxeur d'ENH-008 (`protocol_whitelist=file`, AVIOContext Win32) ; `nvml.dll` seulement via `GetModuleHandleW` (déjà chargée depuis System32, aucun chargement par nom) |
| gitleaks 8.30.1 (`.gitleaks.toml`) | 7 commits de la branche + `docs/qa/resultats/2026-10-24_ENH-047.md`, `…_1854_…`, `…_1919_…`, `docs/reviews/ENH-047.md` : aucune fuite |
| Auteurs / committers | 7/7 = noreply du PO ; chaque message a sa ligne `Agent: gpu-engineer` ; aucune attribution d'outil |
| E-mails / chemins personnels / logiciels du PO | aucun e-mail ; aucun chemin sous le profil utilisateur ; `F:\P153\…` (dossier du projet, convention déjà présente dans `main`) dans `docs/gpu/ENH-047.md` ; rapports QA : **aucune application du PO nommée** (seulement « applications GPU du PO fermées », « VRAM occupée par le bureau ») → aucune correction nécessaire |
| Fichiers interdits | `git ls-files` : aucun `.claude/`, `CLAUDE.md`, `third_party/`, binaire, clip, `.env` ; aucun binaire dans le diff (`--numstat`). **`.obsidian/`** (non suivi, contient `lastOpenFiles`) n'est pas dans `.gitignore` : ne pas l'ajouter |
| Dépendances | aucune nouvelle : `nlohmann-json` et `tl-expected` déjà au manifeste et au registre (`licences.md`, MIT / CC0) ; `psapi`, `dxgi` = bibliothèques système |
| Tests | `enh_bench_unit_tests` : 84/84 (build `dev`) |
| semgrep / cppcheck / osv-scanner | non installés (`pip install semgrep`, `winget install Cppcheck.Cppcheck`, `winget install Google.OSVScanner`) → revue manuelle ciblée |

### Revue manuelle

- **Lecteur BMP** (`bmp.cpp`) : taille du fichier bornée avant lecture (`readFileBounded`, 256 Mio + 4 Kio) ; en-tête ≥ 54 octets vérifié avant toute lecture ; DIB 40/108/124 seulement, troncature de l'en-tête vérifiée ; dimensions 1–8192 (hauteur négative en 64 bits, `INT32_MIN` compris) ; 24/32 bits, `BI_RGB` ou `BI_BITFIELDS` standard (masques lus dans l'en-tête déjà borné) ; foulée, taille et fin des données en arithmétique vérifiée (`checkedMul`/`checkedAdd` en `size_t`) puis comparées à la taille du fichier ; aucun accès hors limites trouvé. L'encodeur borne aussi ses entrées.
- **Vidéos corrompues / tronquées** : code 2 `SourceCorrupt` si paquet corrompu/rejeté ou moins d'images que `nb_frames` ; `--allow-corrupt` mesure quand même avec avertissement. Sans `nb_frames` (mkv, webm), seule la détection par paquets joue (limite documentée par la QA, acceptable pour un banc).
- **Entrée jamais détruite** : `checkDistinctFiles` avant toute ouverture en écriture (`fs::equivalent` sur chemins étendus, repli `weakly_canonical` + comparaison insensible à la casse). **Vérifié** sur le binaire : lien physique vers l'entrée, `IN.BMP`, `sub\..\in.bmp` → code 1, MD5 de l'entrée inchangé. Lien symbolique non testé (privilège absent), couvert par `equivalent` (suit les liens).
- **`OutputFile`** : `CREATE_NEW` par défaut, partage 0, suppression par `FileDispositionInfo` à la destruction sans `commit()` → aucun fichier résiduel (vérifié : BMP invalide → ni `--out` ni fichier partiel). Avec `--force`, l'ancien fichier est tronqué dès l'ouverture (SEC-38).
- **Chemins dans le JSON / stderr** : JSON = noms de fichiers seulement ; `detail` passé par `redactPaths` ; vérifié sous `%TEMP%` du profil : aucun chemin dans le JSON ni sur stderr. `dump(…, replace)` : pas d'exception sur UTF-8 invalide. Limites du masquage : SEC-39.
- **Arguments** : toutes les valeurs numériques bornées (`--size` ≤ côté max, `--repeat`, `--iterations`, `--warmup`, `--sample-ms`) ; tampon hôte de sortie dimensionné par `fitInside` (borné).
- **DLL** : `/DEPENDENTLOADFLAG:0xA00` (dossier de l'exe + System32), pilote préchargé depuis System32 ; pas de `SetDefaultDllDirectories` dans `wmain` (SEC-37).
- **`tests/quality/Measure-Speed.ps1`** : `nvidia-smi` résolu par `Get-Command` (PATH, comme `QaCommon.ps1`), échantillonneur toujours arrêté (`try/finally` + `Stop-Process`), CSV temporaire supprimé, sorties BMP supprimées ; `Get-MachineInfo` n'écrit ni nom de machine ni nom d'utilisateur. Remarque mineure : `Remove-Item $out` sans `-LiteralPath` (noms avec `[` `]`), sans impact sécurité.
- **Hors livrable** : aucune règle `install()`/CPack dans le dépôt ; `package.ps1` n'existe pas encore → consigne SEC-40 pour ENH-030.
- NGX écrit ses données dans `%TEMP%\enhance-bench\ngx` (non nettoyé, outil de dev ; le test ENH-040 « NGX n'écrit ni journal ni mise à jour » reste la garantie).

### Licences

Aucun composant nouveau. `licences.md` inchangé.

### Findings

| Sévérité | Source | Emplacement | Problème | Correction | Quand |
|---|---|---|---|---|---|
| LOW | manuel (confiance élevée) | `src/tools/bench/main.cpp` | SEC-37 : pas de `SetDefaultDllDirectories` / `SetDllDirectoryW(L"")` dans `wmain` | appeler le durcissement d'`enh_platform` | backlog |
| LOW | manuel (confiance élevée) | `src/tools/bench/lib/file_io.cpp` | SEC-38 : `--force` détruit l'ancienne sortie même si la mesure échoue | temporaire + `MoveFileExW(REPLACE_EXISTING)` ; à appliquer surtout aux sorties de l'app | backlog |
| LOW | manuel (confiance moyenne) | `src/tools/bench/lib/file_io.cpp` `redactPaths` | SEC-39 : masquage sensible à la casse et limité aux chemins passés en argument | insensible à la casse + `%TEMP%`/`%USERPROFILE%` | backlog |
| LOW | manuel (confiance élevée) | ENH-030 (`package.ps1`, `check-no-network -Package`) | SEC-40 : rien n'empêche mécaniquement d'embarquer `enhance-bench.exe` et les DLL de dev voisines | liste explicite des fichiers livrés + `forbiddenInPackage` | ENH-030 |
| INFO | manuel | `.gitignore` | `.obsidian/` non ignoré (config locale de l'éditeur, liste de fichiers ouverts) | devops : ajouter `.obsidian/` à `.gitignore` | prochain commit `chore:` |

Threat model : SEC-37 à SEC-40 ajoutés (§ 4).

### Consignes pour devops au merge

1. Commits de la branche : n'ajouter que des fichiers explicitement listés ; **ne pas** ajouter `.obsidian/` ; `docs/qa/broadcast.md` est marqué modifié sans différence de contenu (fins de ligne) : ne pas le committer.
2. Rapports QA non suivis (`docs/qa/resultats/2026-10-24_ENH-047.md`, `…_1854_…`, `…_1919_…`) : relus, committables tels quels si la QA le souhaite.
3. Relancer `scripts/check-no-network.ps1` sur `build/dev` après le merge et garder `enhance-bench.exe` dans `ownBinaries`.
4. ENH-030 : appliquer SEC-40 (aucun fichier de `src/tools/bench/` dans l'installeur ; `-Package` doit échouer s'il y en a un).
