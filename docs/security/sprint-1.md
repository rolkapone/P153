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
