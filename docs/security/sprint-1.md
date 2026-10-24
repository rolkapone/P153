# Sécurité — Sprint 1

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
