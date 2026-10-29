# Enhance — Modèle de menaces

Statut : v1.0 du 2026-10-21 (secdevops), story ENH-006. À relire à chaque ADR qui touche aux entrées, au chargement de code ou au réseau.
Méthode : STRIDE par surface d'attaque, puis findings classés (CRITICAL / HIGH / MEDIUM / LOW) à transformer en stories par le scrum-master.
Avertissement : je ne suis pas juriste ; les points de licence sont dans `licences.md`. Les éléments marqués **[H]** sont des hypothèses à vérifier par test ; le niveau de confiance est indiqué pour chaque finding.

Documents liés : `licences.md` (registre), `zero-reseau.md` (preuve « zéro réseau »), `ci-securite.md` (jobs CI).

---

## 1. Contexte et frontières de confiance

Enhance est une application de bureau Windows mono-utilisateur, sans serveur, sans compte, sans réseau. Elle lit des fichiers vidéo choisis par l'utilisateur, les traite sur le GPU et écrit un nouveau fichier.

```
           (non fiable)                       (fiable = même utilisateur)                 (système / pilote)
┌───────────────────────────┐    ┌──────────────────────────────────────────────┐    ┌─────────────────────────────┐
│ Fichiers vidéo déposés     │──▶│ Enhance.exe (processus utilisateur, asInvoker) │──▶│ Pilote NVIDIA (System32,      │
│ (disque local, clé USB,    │    │  Qt Quick ─ cœur C++ ─ pipeline               │    │ DriverStore) : nvcuda,        │
│ partage SMB, téléchargés)  │    │  FFmpeg (DLL) ─ CUDA ─ NGX/VSR ─ TensorRT     │    │ nvcuvid, nvEncodeAPI64,       │
└───────────────────────────┘    │  ─ NVENC                                       │    │ nvofapi64, NGX Core          │
┌───────────────────────────┐    └───────▲───────────────────────┬───────────────┘    └─────────────────────────────┘
│ Dossier de l'application   │────────────┘                       │
│ (DLL Qt, FFmpeg, SDK)      │  (fiable SI le dossier n'est pas    ▼
└───────────────────────────┘   partagé avec des fichiers tiers) %LOCALAPPDATA%\Enhance\ (settings.json, queue.json,
                                                                  logs\, cache\trt\) — fiable au niveau « même utilisateur »
```

Frontières :
- **F1 — contenu des fichiers vidéo** : totalement non fiable (un fichier peut être forgé pour attaquer un démuxeur ou un décodeur). C'est la frontière principale.
- **F2 — chemins et noms de fichiers** : non fiables (Unicode, longueur, UNC, points d'analyse, noms réservés).
- **F3 — dossier de l'application** : fiable seulement s'il ne contient que nos fichiers (problème des DLL plantées à côté d'un exécutable lancé depuis « Téléchargements »).
- **F4 — données dans le profil** (`%LOCALAPPDATA%\Enhance`) : même niveau de confiance que l'utilisateur ; un attaquant qui peut les écrire a déjà les droits de l'utilisateur. On se protège contre la **corruption** et les **effets de bord dangereux** (suppression de fichiers, chargement de code), pas contre l'utilisateur lui-même.
- **F5 — composants tiers binaires** (pilote, SDK NVIDIA, FFmpeg, Qt) : fiables pour leur code, mais **leur comportement réseau doit être vérifié**, pas supposé (voir SEC-01).

Hors périmètre : attaquant administrateur local, accès physique, compromission du pilote NVIDIA, attaques par canal auxiliaire GPU.

## 2. Actifs

| # | Actif | Pourquoi il compte | Propriété à protéger |
|---|---|---|---|
| A1 | Vidéos privées du PO et des utilisateurs (archives familiales) | Données personnelles, parfois intimes | Confidentialité (**rien ne sort du PC**), intégrité (jamais écrasées) |
| A2 | Chemins et noms des fichiers (révèlent noms de personnes, lieux, dates) | Données personnelles | Confidentialité (pas dans les journaux, le dépôt, les binaires livrés) |
| A3 | Fichiers produits (sorties 4K) et fichiers temporaires | Résultat du travail, plusieurs Gio | Intégrité, pas d'écrasement, pas de fichier partiel trompeur |
| A4 | Journaux (`logs\`), `queue.json`, `settings.json`, cache TensorRT | Contiennent des métadonnées ou du code exécutable (moteur) | Confidentialité, intégrité |
| A5 | Machine du PO (session utilisateur, GPU) | Exécution de code arbitraire = compromission totale | Pas d'exécution de code via un fichier vidéo ou une DLL plantée |
| A6 | Dépôt Git (`rolkapone/P153`) et son historique | Public un jour ; historique indélébile | Aucun secret, aucune adresse e-mail autre que la noreply du PO, aucun fichier interdit |
| A7 | Promesse « 100 % local » | Argument produit n° 1 face au cloud | Preuve vérifiable (voir `zero-reseau.md`) |
| A8 | Conformité des licences | Publication gratuite envisagée | Ne livrer que ce qui est redistribuable (voir `licences.md`) |

## 3. Surfaces d'attaque et analyse STRIDE

Légende STRIDE : **S**poofing (usurpation), **T**ampering (altération), **R**epudiation, **I**nformation disclosure (fuite), **D**enial of service, **E**levation of privilege (exécution de code / élévation).

### 3.1 Fichiers vidéo malformés → FFmpeg (libavformat / libavcodec) et NVDEC

| STRIDE | Menace | Mesures retenues |
|---|---|---|
| E | Débordement de tampon dans un démuxeur ou un décodeur logiciel FFmpeg (historique CVE important : mov, matroska, h264, hevc, av1, sous-titres) → exécution de code dans Enhance | (1) **FFmpeg à jour**, version épinglée et surveillée (job CVE, `ci-securite.md` §6) ; (2) **build de livraison minimal** : seuls les démuxeurs, décodeurs, analyseurs et muxeurs utiles (SEC-06) ; (3) pas de décodeur audio (l'audio est copié, jamais décodé) ; (4) fuzzing d'intégration ENH-015 (§5) ; (5) à terme, sondage dans un processus séparé à privilèges réduits (risque résiduel R-1) |
| E | Fichier qui référence d'autres ressources : MOV `dref` externes, listes HLS/DASH renommées en `.mp4`, scripts `concat`, `file:` / `http:` imbriqués, sous-titres externes | **Ouverture des fichiers par notre couche Win32** (`AVIOContext` maison sur `CreateFileW`, ADR-049) ; **rappel `io_open` remplacé** qui refuse toute ouverture secondaire (sauf la réouverture du fichier de sortie par `faststart`) ; `protocol_whitelist=file` posé quand même (défense en profondeur) ; build de livraison `--disable-network --disable-protocols` (SEC-06). **Constat ENH-008 (2026-10-23)** : certains démuxeurs (`concat`, et par construction `hls`/`dash`/`imf`/`vobsub`) ouvrent leurs fichiers par un **contexte imbriqué** qui ne reprend pas notre `io_open` (seules les listes blanches/noires sont copiées, `ff_copy_whiteblacklists`) : avec `protocol_whitelist=file`, `concat` pourrait lire n'importe quel fichier local. Mesure : **sondage `av_probe_input_buffer2` puis liste blanche de démuxeurs** avant `avformat_open_input` (ADR-049, liste noire en second filet), testée ; à compléter par l'option `format_whitelist` (héritée par les contextes imbriqués, SEC-23) |
| D | Bombes de ressources : dimensions 65535×65535, milliers de pistes, durée de 10 ans, horodatages aberrants, boucles d'index, allocation géante | **Limites** vérifiées dès le sondage, avant toute allocation : largeur et hauteur ≤ 8192 et ≥ 16, pixels ≤ 8192×4320, ≤ 64 pistes (`max_streams`), durée ≤ 24 h, cadence 1–240 i/s, `probesize` ≤ 50 Mio et `analyzeduration` ≤ 30 s, `av_max_alloc` ≤ 1 Gio ; échéance dure de 2 s du sondage (ADR-051) ; `av_image_check_size2` avant toute allocation d'image ; refus → `UnsupportedFormat` |
| D | Fichier qui fait planter le décodeur NVDEC ou provoque une perte du périphérique | Codes retour CUDA/NVDEC tous vérifiés (`check()`, ADR-047) → `DeviceLost` / `SourceCorrupt` ; la file continue (ENH-015) ; le job suivant recrée le contexte |
| T / E | Calcul de tailles d'image en `int` 32 bits (largeur × hauteur × octets/pixel, pas de ligne, décalage des plans NV12/P010) → dépassement d'entier → tampon GPU trop petit → écriture hors limites | Toute taille en `std::size_t` / `uint64_t`, multiplications vérifiées (petite fonction `checked_mul` testée), dimensions bornées avant calcul (SEC-13) |
| I | Métadonnées du fichier (titre, GPS, nom de l'appareil) recopiées dans le journal | `av_dump_format` interdit ; journal = codes et valeurs techniques, jamais les tags (SEC-14) |

### 3.2 Sous-titres (ASS/SSA, SRT, WebVTT, PGS) et pièces jointes

| STRIDE | Menace | Mesures |
|---|---|---|
| E | Analyseur ASS/SRT de libavcodec (balises de style, lignes géantes) lors de la conversion SRT/WebVTT → `mov_text` (ADR-050) | Conversion limitée à SRT/WebVTT → `mov_text` ; ASS/SSA, PGS, VobSub copiés **sans décodage** vers MKV ; pas de `libass` (aucun rendu de sous-titres) ; limite de taille d'un paquet de sous-titre (64 Kio) et du nombre d'événements ; corpus de fuzzing dédié |
| E | Polices embarquées en pièce jointe MKV (attachments) | Copiées telles quelles, **jamais chargées** (ni par Qt, ni par FreeType) |

### 3.3 Chemins : Unicode, longs, UNC, points d'analyse, noms réservés

| STRIDE | Menace | Mesures |
|---|---|---|
| T | Écrasement d'un fichier existant (sortie, collision de noms entre deux jobs) | `OutputPathPlanner` (suffixes `_2`, `_3`), création en `CREATE_NEW`, renommage final `MoveFileExW` **sans** `MOVEFILE_REPLACE_EXISTING` (ADR-010, ADR-049) |
| T | Lien symbolique ou jonction posé à l'emplacement du temporaire pour faire écrire ailleurs | `CREATE_NEW` échoue si l'entrée existe (y compris un lien) ; le dossier de sortie est vérifié comme répertoire (pas un point d'analyse vers un autre volume inattendu : avertissement seulement) |
| T / D | **Suppression arbitraire** : au démarrage, les temporaires « référencés dans `queue.json` » sont supprimés ; un `queue.json` altéré ou corrompu pourrait désigner n'importe quel fichier | Avant toute suppression : le nom doit se terminer par `.enhance-tmp.<mp4|mkv>`, être un fichier régulier (`FILE_ATTRIBUTE_REPARSE_POINT` absent), et le chemin doit égaler exactement celui calculé par `OutputPathPlanner` pour ce job (SEC-11) |
| I / S | Chemin **UNC** (`\\hôte\partage\...`) rouvert automatiquement au démarrage (file persistée, vignettes, sondage) : connexion SMB non initiée par l'utilisateur à ce moment, avec envoi d'un défi NTLM vers un hôte éventuellement externe | Au redémarrage, pas de sondage ni de vignette automatique pour les chemins UNC : la carte s'affiche avec le nom seul, le sondage reprend au démarrage effectif du job ; refus des chemins `\\?\UNC\` vers une adresse IP publique **[H]** à discuter ; voir SEC-10 |
| D | Chemins > 260 caractères, Unicode, espaces, noms réservés (`CON`, `NUL`), point ou espace final, flux de données alternés (`fichier.mp4:flux`) | Préfixe `\\?\` (ADR-049) ; refus des noms contenant `:` hors lettre de lecteur ; tests unitaires (ADR-010) étendus aux noms réservés et aux ADS |
| D | Disque plein, lecteur retiré, partage déconnecté en cours d'écriture | Espace vérifié avant (ADR-010) ; erreurs Win32 traduites (`DiskFull`, `SourceMissing`) ; le temporaire est fermé puis supprimé par l'application |

### 3.4 Glisser-déposer et actions Shell

| STRIDE | Menace | Mesures |
|---|---|---|
| S / I | Dépôt d'une **URL** (glissée depuis un navigateur : `https://...`) ou d'un raccourci `.lnk` / `.url` | `QUrl::isLocalFile()` obligatoire, sinon refus silencieux avec message ; `.lnk`, `.url`, `.scf`, `.library-ms` refusés (pas de résolution de raccourci au MVP) ; dossiers refusés au MVP (ENH-037 plus tard) |
| E | « Lire » / « Afficher dans le dossier » : `ShellExecuteExW` sur un chemin qui serait une URL ou un exécutable | `ShellExecuteExW` uniquement sur le **fichier produit par Enhance** (extension `.mp4`/`.mkv` vérifiée, chemin local existant), verbe `open` explicite ; jamais sur un chemin venu tel quel du QML |
| E | Application lancée en administrateur : fichiers créés avec un propriétaire administrateur, glisser-déposer bloqué par UIPI | Manifeste `asInvoker` ; si le jeton est élevé, bandeau d'avertissement (SEC-20) |

### 3.5 Fichiers JSON (préréglages, file, réglages)

| STRIDE | Menace | Mesures |
|---|---|---|
| T / D | `queue.json` / `settings.json` tronqué, géant, profondément imbriqué, valeurs hors bornes ; préréglages personnalisés importés (ENH-034, plus tard) = **entrée non fiable** | Écriture atomique (ADR-006) ; lecture avec taille maximale (1 Mio), profondeur maximale (32), exceptions `nlohmann::json` attrapées au bord (ADR-003), **validation par schéma** (bornes de débit, résolutions, booléens), valeurs inconnues ignorées ; fichier invalide → valeurs par défaut + `.bak` (SEC-18) |
| I | `queue.json` contient les chemins complets | Reste dans `%LOCALAPPDATA%` (ADR-006) ; jamais journalisé ; jamais inclus dans un rapport ou une capture |

### 3.6 Chargement de DLL (détournement de l'ordre de recherche)

| STRIDE | Menace | Mesures |
|---|---|---|
| E | DLL plantée dans le **dossier courant** ou le `PATH` (`nvcuda.dll`, `dwmapi.dll`, `version.dll`, `d3dcompiler_47.dll`...) | Première instruction de `wWinMain` : `SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_APPLICATION_DIR)` + `SetDllDirectoryW(L"")` + `SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT)` ; option d'éditeur de liens **`/DEPENDENTLOADFLAG:0xA00`** pour les imports statiques résolus avant `main` (SEC-08) |
| E | DLL plantée **à côté de l'exécutable** (ZIP portable décompressé directement dans « Téléchargements ») | Le ZIP contient un dossier racine `Enhance-x.y.z\` ; le `LISEZ-MOI` demande de l'extraire dans un dossier dédié ; l'installeur (plus tard) installe dans `%LOCALAPPDATA%\Programs\Enhance` |
| E | DLL du **pilote** NVIDIA (`nvcuda`, `nvcuvid`, `nvEncodeAPI64`, `nvofapi64`, `nvml`) chargées par nom par `cudart` statique ou par FFmpeg : avec `APPLICATION_DIR`, le dossier de l'application passe **avant** System32 | **Préchargement explicite** depuis System32 avant tout appel CUDA/FFmpeg : `LoadLibraryExW(L"nvcuda.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)` (idem pour les quatre autres) ; les chargements ultérieurs par nom retrouvent le module déjà chargé ; test qui vérifie le chemin réel des modules chargés (`GetModuleFileNameW`) |
| E | DLL des SDK (`nvngx_vsr.dll`, TensorRT, FRUC) et pont `enh_rtxvideo_bridge.dll` | Chargement par **chemin absolu** avec `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32` ; uniquement les DLL **`rel`** du RTX Video SDK (jamais `dev`, contrôlé par `ngx_static_policy_test`) ; pont : `LoadLibraryExW(<dossier de l'exe>\enh_rtxvideo_bridge.dll)`, 7 `GetProcAddress` + contrôle d'ABI, `/DEPENDENTLOADFLAG:0x800`. **NGX n'est pas un chargeur ordinaire** (ENH-040) : `_nvngx.dll` vient du DriverStore, la DLL de fonction est projetée sans le chargeur Windows et peut être une copie du magasin `%ProgramData%\NVIDIA\NGX\models` (`PathListInfo` vide n'y change rien) → `nvngx_vsr.dll` du dossier de l'application vérifiée **avant** (Authenticode « NVIDIA Corporation », hors ligne) et DLL réellement utilisée vérifiée **après** chargement (journal NGX en mémoire ; règle PO du 2026-10-24 : la nôtre, copie identique, ou autre DLL signée NVIDIA avec avertissement ; sinon refus) ; risque résiduel R-7, SEC-31. `nvcuda.dll` : préchargée depuis System32 (SEC-35) |
| E | Plugins Qt chargés depuis un chemin inattendu (`QT_PLUGIN_PATH`, préfixe compilé dans `Qt6Core.dll` pointant vers un dossier de build, `qt.conf` absent) | `qt.conf` livré (`[Paths] Prefix=.`) ; `QCoreApplication::setLibraryPaths({<app>/plugins})` **avant** la création de `QGuiApplication` ; variables `QT_PLUGIN_PATH`, `QT_QPA_PLATFORM_PLUGIN_PATH`, `QML_IMPORT_PATH`, `QML2_IMPORT_PATH` ignorées en build release (effacées au démarrage) |

### 3.7 Moteur TensorRT en cache et modèle d'interpolation

| STRIDE | Menace | Mesures |
|---|---|---|
| E | Un moteur TensorRT sérialisé est du **code** (noyaux, et pour les moteurs « version-compatible » du code hôte) ; un fichier de cache remplacé → exécution de code | `IRuntime::setEngineHostCodeAllowed(false)` ; pas de moteurs version-compatible ni « lean » embarquant un runtime ; aucun `IPluginRegistry::loadLibrary` (aucun plugin chargé par chemin) ; index du cache avec SHA-256 du moteur et clé (version TensorRT, `sm_120`, pilote, SHA-256 de l'ONNX) ; incohérence → reconstruction (SEC-12) |
| T | Fichier ONNX livré altéré | SHA-256 de l'ONNX compilé dans l'exécutable, vérifié avant la construction du moteur |
| E | Poste du PO : `flownet.pkl` (format pickle) téléchargé depuis un lien de partage → exécution de code à la désérialisation pendant l'export ONNX | SHA-256 consignée au premier téléchargement ; `torch.load(..., weights_only=True)` ; venv dédié hors dépôt ; export jamais lancé en CI |
| T / licence | ONNX tiers de provenance incertaine (ex. vs-mlrt, GPL-3.0) livré à la place de l'export maison | Verrou des modèles (SHA-256 de l'ONNX exporté par nous) ; `package.ps1` refuse tout ONNX dont la somme n'est pas celle du verrou |
| D | Construction du moteur longue (1–3 min) ou qui sature la VRAM | Étape `PreparingModels` visible, annulable ; espace de travail TensorRT borné (budget VRAM ADR-043) |

### 3.8 Réseau (exigence ferme « zéro réseau »)

Détail complet et méthode de preuve : `zero-reseau.md`.

| STRIDE | Menace | Mesures |
|---|---|---|
| I | Code d'Enhance ou d'une bibliothèque qui ouvre une connexion (télémétrie, mise à jour, téléchargement de modèle) | Aucune API réseau dans `src/` (contrôle CI), Qt **sans** `Qt6Network` (ADR-024), FFmpeg de livraison `--disable-network` |
| I | **Mise à jour à la volée de NGX** (RTX Video SDK) : le « NGX Update Module » (guide chap. 1, p. 7) ; **constaté en ENH-040** : à chaque `NVSDK_NGX_CUDA_Init`, même avec `AppId = 0`, NGX lance `nvngx_update.exe -api update … -bootstrap` (4 processus ; l'exécutable importe `WS2_32`, `CRYPT32`), journal `OTAEnabled = 1` | SEC-01 : jamais `NVSDK_NGX_UpdateFeature` ni Init par identifiant de projet (C1 + `ngx_static_policy_test`) ; `AppId = 0` sans `ProjectIdDescription` ; **processus enfants interdits** pendant `rvb_runtime_create` et `rvb_vsr_create` (Job Object, `ActiveProcessLimit = 1` ; QA : 0 `nvngx_update.exe` sur 15 inits, sondage toutes les 2 ms) ; DLL de fonction vérifiée après chargement (§ 3.6) ; échec ouvert du Job Object à corriger (SEC-30) ; preuve réseau sur session complète : ENH-041 |
| I | Vérification de signature (NVIDIA : `cryptnet.dll`, `wldp.dll` visibles dans `nvngx_vsr.dll` ; Enhance : `WinVerifyTrust` sur la DLL de fonction) → récupération de CRL/OCSP en ligne | SEC-07 : côté Enhance, `WTD_REVOKE_NONE` + `WTD_REVOCATION_CHECK_NONE` + `WTD_CACHE_ONLY_URL_RETRIEVAL` (aucune URL récupérée, SEC-34) ; côté NVIDIA : test dynamique M2 (ENH-041) ; si constaté, documenter l'exception (trafic de Windows, pas de données utilisateur) ou demander à NVIDIA |
| I | Rapports d'erreurs Windows (WER) envoyés à Microsoft avec un vidage mémoire contenant des chemins | SEC-14 : décision PO (exclusion via `WerAddExcludedApplication`, vidage local uniquement) |
| I | Accès SMB aux partages choisis par l'utilisateur | Exception documentée : trafic du redirecteur Windows (processus `System`), à la demande explicite de l'utilisateur ; pas de reconnexion automatique (SEC-10) |

#### 3.8.1 NGX : constats et mesures d'ENH-040 (2026-10-24)

| Constat (pilote 617.42, RTX 5070) | Mesure dans Enhance | État |
|---|---|---|
| `nvngx_update.exe` lancé à chaque init NGX malgré `AppId = 0` | Job Object `ActiveProcessLimit = 1` pendant l'init et la création de l'effet ; processus rattaché à vie, limite levée hors de cette fenêtre (contrainte acceptée par le PO : aucun processus lancé par l'app pendant ~1 s) | Fait ; QA : 0 processus sur 15 inits ; échec ouvert si le job est indisponible (SEC-30) |
| DLL de fonction exécutée = copie du magasin NGX (identique à la nôtre sur le poste du PO : `NgxStoreIdentical 1.6.0`) | Vérification après chargement : la nôtre / copie identique → OK ; autre DLL signée « NVIDIA Corporation » (Authenticode hors ligne) → OK + avertissement `VsrFeatureNotShipped` (version + empreinte SHA-256, sans chemin) ; sinon `RtxVideoFeatureUntrusted` | Fait (décision PO) ; détection seulement (R-7) ; magasin inscriptible par tous (SEC-31) |
| Journal NGX nécessaire pour connaître la DLL utilisée | Niveau ON, `DisableOtherLoggingSinks = true`, lignes analysées en mémoire, renvoyées au journal d'Enhance seulement si l'application le demande ; test GPU : 0 fichier écrit dans le dossier de données NGX, magasin système inchangé | Fait |
| Télémétrie `NGXSendTelemetryInitializeData` à l'init | Aucune prise côté Enhance (pas d'API d'opt-out documentée) | **Ouvert** : à mesurer en ENH-041 (R-8) |
| 27 exports `NVSDK_NGX_*` (dont `UpdateFeature`) ré-exportés par le pont | Jamais résolus ; appels interdits par contrôle statique | Accepté (SEC-32) |

### 3.9 Journaux et confidentialité

| STRIDE | Menace | Mesures |
|---|---|---|
| I | Chemins complets, noms de fichiers, métadonnées dans les journaux d'Enhance ou des tiers (`av_log`, journal NGX, journal TensorRT, messages Qt) | `log::fileNameOnly` (ADR-007) ; `av_log` filtré (niveau ≤ `AV_LOG_WARNING`, pas d'URL/chemin) ; NGX initialisé avec `InApplicationDataPath = %LOCALAPPDATA%\Enhance\ngx` et `DisableOtherLoggingSinks = true` : NGX n'écrit aucun fichier (vérifié par test GPU, ENH-040) ; son journal (niveau ON) est lu **en mémoire** pour identifier la DLL de fonction et n'est renvoyé au journal d'Enhance que sur demande (mode diagnostic : il contient alors des chemins du profil, SEC-14) ; messages de refus et avertissements sans chemin (version + empreinte) ; journal TensorRT redirigé avec niveau `kWARNING` ; mode `--verbose` documenté comme « contient des chemins » (SEC-14) |
| I | Chemins de build ou du profil du PO compilés dans les binaires livrés (chemin PDB, `__FILE__`, préfixe Qt) | `/PDBALTPATH:%_PDB%`, `std::source_location` tronqué au dossier `src/`, contrôle de chaînes `C:\Users\` dans le livrable (SEC-16) |

### 3.10 Élévation, installeur, mises à jour

| STRIDE | Menace | Mesures |
|---|---|---|
| E | Installeur exécuté en administrateur depuis « Téléchargements » (préchargement de DLL de l'installeur) | Installeur Inno Setup (ADR-033) **par utilisateur**, `PrivilegesRequired=lowest`, version d'Inno Setup à jour ; aucune élévation nécessaire |
| T | Binaires non signés modifiés en transit | SHA-256 publiée à côté du ZIP (ADR-032) ; signature à étudier avant toute diffusion (ADR-034, SEC-19) |
| T | Désinstallation qui supprime des fichiers utilisateur | Le désinstalleur ne touche qu'au dossier d'installation ; `%LOCALAPPDATA%\Enhance` supprimé seulement sur demande ; jamais les vidéos |
| — | Mise à jour automatique | **Interdite** (exigence produit) ; mises à jour manuelles par nouveau ZIP / installeur |

### 3.11 Chaîne d'approvisionnement et dépôt

| STRIDE | Menace | Mesures |
|---|---|---|
| T | Dépendance compromise ou non épinglée (vcpkg, FFmpeg BtbN, actions GitHub) | Baseline vcpkg épinglée par SHA (ADR-023), FFmpeg par tag + SHA-256 (ADR-025), actions par SHA de commit (ADR-029), outils CI téléchargés avec somme vérifiée |
| I | Secret, adresse e-mail professionnelle, chemin personnel ou fichier interdit dans le dépôt / l'historique | gitleaks + règle e-mail (liste blanche = noreply du PO), contrôle des auteurs de commits, `git ls-files` contre la liste des fichiers interdits (`ci-securite.md`), hook de pré-commit |
| I | **Configuration git globale de la machine contenant l'adresse professionnelle du PO** (constaté le 2026-10-21 ; la config locale du dépôt est correcte) : un nouveau clone, un worktree ou un outil qui ignore la config locale commiterait avec cette adresse | SEC-05 : action PO (voir §4) ; contrôle CI des auteurs ; hook de pré-commit |

### 3.12 Interface Qt Quick et intégration Windows (ENH-004)

| STRIDE | Menace | Mesures |
|---|---|---|
| T / E | **Cache de pipeline de Qt Quick** écrit et relu au démarrage dans `%LOCALAPPDATA%\Enhance\cache\qtpipelinecache-<arch>\` (shaders compilés D3D11) : fichier du profil altéré | Même niveau de confiance que l'utilisateur (F4, R-3) ; Qt rejette un cache dont l'en-tête ne correspond pas au périphérique ou au pilote ; jamais sous le dossier d'installation ; SEC-27 |
| T | Valeurs du registre lues au démarrage et à chaque `WM_SETTINGCHANGE` : `HKCU\...\Themes\Personalize` (`AppsUseLightTheme`, `EnableTransparency`) et `HKCU\Software\Microsoft\Windows\DWM\ColorPrevalence` | Lecture seule, `RegGetValueW` avec `RRF_RT_REG_DWORD` (type et taille imposés), valeur absente → défaut ; ne pilote que l'apparence (aucun chemin, aucun code) ; rafales de messages regroupées (minuterie) |
| D | Shader `glass.frag` (compilé à la construction par `qsb`, embarqué en ressource) | Aucune donnée externe : uniformes posés par notre QML, texture = capture de notre propre fenêtre ; aucune boucle dépendant des données ; divisions protégées (`max(..., 1e-3)`) ; au pire un pixel faux. Gouverneur de qualité (repli vers l'opaque) si le temps GPU dépasse le budget |
| I | Captures d'écran et rapports QA de l'interface | Fenêtre d'Enhance seule, aucun autre logiciel ni bureau visible ; pas de nom de machine, de compte ni de logiciel personnel dans les rapports (SEC-25, SEC-29) |
| I | URL présentes dans le binaire | Seulement 3 identifiants d'espace de noms XML (manifeste, icône SVG), jamais résolus ; exceptions exactes U1–U3 de `scripts/network-allowlist.json` (SEC-26) |
| E | Lancement en administrateur | Manifeste `asInvoker`, bandeau si jeton élevé (SEC-20, clos) |


## Sommaire (découpage du 2026-10-29)

- § 1-3 contexte, actifs, STRIDE : [threat-model.md](threat-model.md)
- § 4 Findings : [threat-model-findings.md](threat-model-findings.md)
- § 5-7 fuzzing, risques résiduels, exigences : [threat-model-fuzzing-risques.md](threat-model-fuzzing-risques.md)
