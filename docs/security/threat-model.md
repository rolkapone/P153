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
| E | DLL des SDK (`nvngx_vsr.dll`, TensorRT, FRUC) | Chargement par **chemin absolu** (`<dossier app>\nvidia\...`) avec `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32` ; `NVSDK_NGX_FeatureCommonInfo::PathListInfo` limité à ce dossier ; uniquement les DLL **`rel`** du RTX Video SDK (jamais `dev`) ; signature Authenticode NVIDIA vérifiée en test (`Get-AuthenticodeSignature`) |
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
| I | **Mise à jour à la volée de NGX** (RTX Video SDK) : le guide du SDK (chap. 1, p. 7) indique que le « NGX Update Module » **télécharge et remplace** la DLL de la fonction quand l'application l'initialise | SEC-01 : ne jamais appeler `NVSDK_NGX_UpdateFeature` ; identifiant d'application `0` sans `ProjectIdDescription` (l'en-tête `nvsdk_ngx_defs.h` l. 446 indique que l'identifiant est « requis pour les mises à jour over the air ») ; **test dynamique obligatoire** (pare-feu + audit WFP + capture) pendant ENH-001 ; vérifier que la DLL réellement chargée est celle du dossier de l'application |
| I | Vérification de signature par les DLL NVIDIA (`cryptnet.dll`, `wldp.dll` visibles dans `nvngx_vsr.dll`) → récupération de CRL/OCSP en ligne | SEC-07 : test dynamique ; si constaté, documenter l'exception (trafic de Windows, pas de données utilisateur) ou demander à NVIDIA |
| I | Rapports d'erreurs Windows (WER) envoyés à Microsoft avec un vidage mémoire contenant des chemins | SEC-14 : décision PO (exclusion via `WerAddExcludedApplication`, vidage local uniquement) |
| I | Accès SMB aux partages choisis par l'utilisateur | Exception documentée : trafic du redirecteur Windows (processus `System`), à la demande explicite de l'utilisateur ; pas de reconnexion automatique (SEC-10) |

### 3.9 Journaux et confidentialité

| STRIDE | Menace | Mesures |
|---|---|---|
| I | Chemins complets, noms de fichiers, métadonnées dans les journaux d'Enhance ou des tiers (`av_log`, journaux NGX dans `InApplicationDataPath`, journal TensorRT, messages Qt) | `log::fileNameOnly` (ADR-007) ; `av_log` filtré (niveau ≤ `AV_LOG_WARNING`, pas d'URL/chemin) ; NGX initialisé avec `InApplicationDataPath = %LOCALAPPDATA%\Enhance\ngx` et journalisation NGX désactivée en release ; journal TensorRT redirigé avec niveau `kWARNING` ; mode `--verbose` documenté comme « contient des chemins » (SEC-14) |
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

## 4. Findings

Format : identifiant, sévérité, confiance, composant, problème, correction recommandée, propriétaire proposé, story. Les HIGH sont **bloquants** pour la story indiquée.

| ID | Sév. | Confiance | Emplacement | Problème | Correction recommandée | Pour | Story |
|---|---|---|---|---|---|---|---|
| SEC-01 | **HIGH** | Moyenne (comportement documenté par NVIDIA ; déclenchement avec `AppId=0` non vérifié) | RTX Video SDK 1.1.0 — `doc/NVIDIA_RTX_Video_SDK_Programming_Guide.pdf` chap. 1 p. 7 ; `include/nvsdk_ngx.h` l. 491 ; `nvsdk_ngx_defs.h` l. 446 | Le « NGX Update Module » peut télécharger une nouvelle DLL de la fonction lors de l'initialisation : contraire à « zéro réseau » et remplace silencieusement le code exécuté | Ne jamais appeler `NVSDK_NGX_UpdateFeature` ; `InApplicationId = 0`, pas de `ProjectIdDescription` ; vérifier au chargement le chemin et la signature de `nvngx_vsr.dll` réellement utilisée ; **test dynamique** M1–M3 de `zero-reseau.md` pendant ENH-001 ; si un trafic est constaté : escalade PO + question à NVIDIA (contact de l'art. 19 de la licence RTX) | gpu-engineer + secdevops | ENH-001, ENH-028 |
| SEC-02 | **LOW** (licence ; HIGH puis MEDIUM, **levé** le 2026-10-21, reste des obligations de livraison) | Élevée (texte du SLA lu ; confirmation PO) | `third_party/nvidia/tensorrt` (TensorRT 11.4.0.106 « Enterprise ») | Licence applicable : SLA SDK NVIDIA (v. 2021-05-24) + Supplément TensorRT (v. 2021-02-25) ; HTML SHA-256 `ee9cc052…46c4`, PDF hors dépôt `third_party/nvidia/_licences/TensorRT-SLA_2026-10-21.pdf` SHA-256 `360a76c3…2a890`. PO : aucune autre condition acceptée pour l'édition « Enterprise ». Usage local autorisé (1.1 (i), Suppl. 1) ; DLL d'exécution redistribuables (Suppl. 2) sous conditions 1.2 et 2.1–2.5 | Livraison (ENH-030) : seulement `nvinfer_11`, `nvinfer_builder_resource_sm120_11`, `nvonnxparser_11` (+ `nvinfer_plugin_11` si utilisé), non modifiées, chargées par chemin absolu par Enhance seul, + `Acknowledgements.txt` ; jamais `trtexec`, `.lib`, Python ; pas de moteur pré-construit livré ; CLUF Enhance (SEC-21) reprenant l'art. 1.2 | devops + secdevops | ENH-030 |
| SEC-03 | **MEDIUM** (licence ; était HIGH, révisé le 2026-10-21) | Moyenne (citation du README relevée par le gpu-engineer, non relue par moi faute de réseau) | Poids RIFE 4.25 (Practical-RIFE) et ONNX dérivés | Le README de `hzwer/Practical-RIFE` (MIT, commit `bbfd2ea9`) place les poids « under the same MIT license as this project » → **poids livrables** (MIT). L'ONNX de vs-mlrt (`rife_v4.25.7z`, dépôt GPL-3.0, sans licence propre aux fichiers, graphe « v2 » de vs-mlrt) est **écarté** de la livraison, gardé pour l'essai local. Risque résiduel : données d'entraînement Vimeo-90K (usage de recherche) | **Export ONNX maison** depuis `flownet.pkl` : script écrit par nous à partir du code MIT (sans reprendre le script ni l'enveloppe « v2 » de vs-mlrt), outil local Python 3.12 + PyTorch (roues CPU suffisantes ; cu128 facultatif) épinglé avec `--require-hashes`, rien de Python livré ; `torch.load(weights_only=True)` ; SHA-256 du `.pkl` et de l'ONNX produit consignées ; README + `LICENSE` archivés ; mention MIT livrée. Le PO confirme la lecture du README et accepte le risque Vimeo-90K (`licences.md` §7.2) | gpu-engineer + PO | ENH-002, ENH-030 |
| SEC-04 | **MEDIUM** (licence ; était HIGH, révisé le 2026-10-21) | Élevée (texte lu) | Optical Flow SDK 5.0.7 — `LicenseAgreement.pdf` art. 1.c ; `NvOFFRUC.dll` | La licence n'autorise à distribuer **que le code source d'exemple** ; `NvOFFRUC.dll` est un binaire, donc **non redistribuable** ; dépend de `cudart64_110.dll` (CUDA 11). Depuis que RIFE est livrable (SEC-03), FRUC n'est plus qu'un **simple repli local** : le risque restant est de le livrer par erreur | FRUC derrière une option de build / un chemin local, jamais requis par l'application ; `package.ps1` refuse `NvOFFRUC.dll` et `cudart64_110.dll` dans le livrable (liste de fichiers interdits du contrôle de licences) ; accord écrit de NVIDIA seulement si le PO veut un jour le livrer | gpu-engineer + devops | ENH-002, ENH-030 |
| SEC-05 | **HIGH** (processus) | Élevée (constaté) | Configuration git **globale** de la machine | Contient l'adresse professionnelle du PO (non recopiée ici) ; seule la config locale du dépôt protège | PO : remplacer l'adresse globale par la noreply (ou la supprimer et utiliser `includeIf` pour les dépôts professionnels) ; activer sur GitHub « Keep my email addresses private » et « Block command line pushes that expose my email » ; hook de pré-commit + job CI `identity` | PO + devops | ENH-003 |
| SEC-06 | **HIGH** (porte de release) / MEDIUM pendant le spike | Élevée | FFmpeg BtbN `win64-lgpl-shared` (ADR-025) | Build générique : tous les démuxeurs/décodeurs, protocoles réseau (import `ws2_32`), `avdevice` (capture écran/caméra), bibliothèques externes : surface d'attaque maximale | Spike : accepté avec AVIO maison + `io_open` restrictif + `protocol_whitelist=file` ; exceptions **dev-only** datées E3, E3c, E3d, E3e de `scripts/network-allowlist.json` (constat du 2026-10-21 : `avformat`, mais aussi `avcodec` (`ws2_32` + `dnsapi`) et `avfilter` (`ws2_32`, libzmq) importent des sockets complets ; échec garanti en mode `-Package`, échéance 2027-01-21). **SEC-06 reste obligatoire avant ENH-030** : port vcpkg (ou overlay) avec `--disable-network --disable-protocols --disable-avdevice --disable-devices --disable-autodetect --disable-encoders --enable-encoder=mov_text`, démuxeurs/décodeurs/muxeurs en liste blanche (à établir avec le gpu-engineer), aucun décodeur audio, sans `gpl`/`nonfree` | gpu-engineer + devops | ENH-015, ENH-030 |
| SEC-07 | MEDIUM | Faible (indice : `cryptnet.dll`, `wldp.dll`, chaînes CRL/OCSP dans `nvngx_vsr.dll`) | `nvngx_vsr.dll` (vérification de signature du cœur NGX) | Une vérification Authenticode avec contrôle de révocation peut télécharger une CRL/OCSP | Test dynamique M2 ; si confirmé : exception documentée (trafic système sans donnée utilisateur) ou contournement validé par le PO | secdevops | ENH-001 |
| SEC-08 | MEDIUM | Élevée | `main.cpp`, édition de liens, chargement des SDK, Qt | Ordre de recherche des DLL non durci (aucun code encore) | §3.6 : `SetDefaultDllDirectories` en premier, `/DEPENDENTLOADFLAG:0xA00`, préchargement System32 des DLL pilote, SDK par chemin absolu, `qt.conf` + `setLibraryPaths`, ZIP avec dossier racine ; test automatique des chemins des modules chargés | developer + gpu-engineer | ENH-004, ENH-001 |
| SEC-09 | MEDIUM | Élevée | `IMediaProbe` (sondage et vignettes) | Analyse automatique, dans le processus principal, de tout fichier déposé (non fiable) ; un plantage FFmpeg arrête l'application, une faille donne l'exécution de code | Limites §3.1 ; fuzzing d'intégration (§5) dans ENH-015 ; story « plus tard » : sondage dans un processus enfant à jeton restreint (AppContainer ou niveau d'intégrité bas), redémarré à chaque crash | gpu-engineer | ENH-015 (+ story à créer) |
| SEC-10 | MEDIUM | Moyenne | Restauration de `queue.json` au démarrage | Chemins UNC rouverts automatiquement (sondage, vignette) : connexion SMB / défi NTLM non initiés par l'utilisateur ; rend aussi la preuve « zéro réseau » ambiguë | Pas de sondage ni de vignette automatiques pour les chemins UNC au redémarrage ; accès seulement au démarrage du job ; exception SMB documentée dans `zero-reseau.md` | developer | ENH-022 |
| SEC-11 | MEDIUM | Élevée | Nettoyage des temporaires (ADR-006, ADR-010) | Suppression d'un chemin lu dans `queue.json` sans validation → suppression arbitraire si le fichier est corrompu ou altéré | Valider : suffixe `.enhance-tmp.<ext>`, fichier régulier sans point d'analyse, égalité avec le chemin recalculé ; tests unitaires | developer | ENH-022, ENH-024 |
| SEC-12 | MEDIUM | Moyenne | Cache `%LOCALAPPDATA%\Enhance\cache\trt\` (ADR-043) | Moteur désérialisé = code ; plugins TensorRT chargeables par chemin | `setEngineHostCodeAllowed(false)`, pas de moteur version-compatible, pas de `loadLibrary` de plugins, SHA-256 du moteur et de l'ONNX dans l'index, reconstruction si écart | gpu-engineer | ENH-012 / ENH-002 |
| SEC-13 | MEDIUM | Moyenne | Pipeline (allocations de surfaces, noyaux CUDA, NVENC) | Dépassement d'entier sur largeur × hauteur × octets, pas de ligne, décalages de plans | Dimensions bornées au sondage, `checked_mul` (`std::size_t`), `av_image_check_size2`, tests sur 1×1, 8192×4320, largeurs impaires | gpu-engineer | ENH-015 |
| SEC-14 | MEDIUM | Moyenne | Journaux tiers, WER | Fuite de chemins / métadonnées (NGX, TensorRT, `av_log`, Qt) ; vidages WER envoyés par Windows | §3.9 ; décision PO sur WER (recommandation : `WerAddExcludedApplication` + vidage local optionnel dans `logs\`) | developer + gpu-engineer + PO | ENH-025 |
| SEC-15 | LOW | Élevée | Glisser-déposer, `ShellExecuteExW` | URL, raccourcis, dossiers ; ouverture d'un chemin non maîtrisé | §3.4 | developer | ENH-016 / ENH-023 |
| SEC-16 | LOW | Élevée | Binaires livrés | Chemins de build / profil compilés (PDB, `__FILE__`, préfixe Qt) | `/PDBALTPATH:%_PDB%`, troncature de `source_location`, contrôle de chaînes dans `check-no-network.ps1` (étape « chaînes ») | devops | ENH-030 |
| SEC-17 | LOW | Élevée | ADR-025 | `third_party/ffmpeg.lock.json` doit être versionné mais `third_party/` est ignoré et interdit en CI | Déplacer en `deps/ffmpeg.lock.json` (ou `cmake/`) | devops | ENH-003 |
| SEC-18 | LOW | Élevée | Lecture JSON | Pas encore de bornes (taille, profondeur, schéma) | §3.5 | developer | ENH-018, ENH-022 |
| SEC-19 | LOW | Élevée | Livraison | Binaires non signés, SmartScreen | SHA-256 publiée ; ADR-034 avant diffusion publique | devops + PO | après MVP |
| SEC-20 | LOW | Élevée | Démarrage | Exécution en administrateur non détectée | Manifeste `asInvoker`, bandeau si jeton élevé | developer | ENH-004 |
| SEC-21 | À confirmer (licence) | — | Licence d'Enhance elle-même | Les licences NVIDIA (art. 2.c RTX, 2.d DesignWorks, 1.1.2.5 CUDA) exigent des conditions de distribution « au moins aussi protectrices » ; la LGPL exige d'autoriser l'ingénierie inverse pour déboguer les modifications des bibliothèques ; Enhance ne doit pas être publié sous GPL (art. 4.e RTX) | Rédiger un court CLUF / fichier `LICENSE` d'Enhance (gratuiciel propriétaire ou licence permissive) compatible avec les deux ; décision PO | PO + secdevops | ENH-030 |
| SEC-22 | LOW | Élevée (constaté par `objdump -p` le 2026-10-21) | `Qt6Core.dll` 6.11.2 (vcpkg, sans Network) | Importe `ws2_32` #101 `WSAAsyncSelect`, #115 `WSAStartup`, #116 `WSACleanup` (boucle d'événements, aucune socket créée) ; importe aussi `mpr` (`WNetGetUniversalNameW`) et `netapi32` (`NetShareEnum`) pour les chemins réseau Windows choisis par l'utilisateur | Exception E2 limitée à ces 3 fonctions (tout autre import `ws2_32` fait échouer C3), revalidation à chaque version de Qt et au plus tard le 2027-04-21 ; avertissements `mpr`/`netapi32` acceptés (`acknowledgedWarnings`) ; règle de code : ne jamais énumérer de partages réseau de façon proactive | secdevops | ENH-003 |
| SEC-23 | MEDIUM | Élevée (code lu) | `ffmpeg_decoder.cpp` (`FfmpegDecoder::create`), `ffmpeg_demuxer.cpp` (ENH-008) | Pas de liste blanche de **décodeurs** : un flux `SourceCodec::Other` (ou un nom absent de la build) retombe sur `avcodec_find_decoder`, donc sur n'importe quel décodeur de la build (BtbN : des centaines, dont libaom, libvpx, openjpeg, jxl…) ; liste blanche des démuxeurs faite en C++ seulement (pas d'option `format_whitelist`, qui protège aussi les contextes imbriqués) ; pas de `AVCodecContext::max_pixels` (le décodeur alloue avant notre contrôle des dimensions dans `deliver`) | Refuser `SourceCodec::Other` (ou liste blanche explicite de décodeurs : h264, hevc, av1/libdav1d, vp9, vp8, mpeg1/2video, mpeg4, vc1/wmv3, prores, ffv1 + DNxHD si MXF est gardé) et poser `codec_whitelist` sur le contexte ; poser `format_whitelist` = même liste que `isAllowedDemuxer` ; `max_pixels = 8192*4320` ; tests unitaires | gpu-engineer | ENH-015 |
| SEC-24 | MEDIUM | Élevée (constaté) | `scripts/check-no-network.ps1` (C1, exigences positives) | Le contrôle cherche le module FFmpeg dans `src/pipeline/gpu/ffmpeg` alors qu'il est dans `src/pipeline/ffmpeg` : les exigences positives (`protocol_whitelist`, `io_open`) sont « non évaluées » sans échec ; une régression (retrait de `io_open` ou de la liste blanche) passerait la CI | Chercher tout fichier de `src/` qui appelle `avformat_open_input` et exiger dans ce même module `protocol_whitelist`, `io_open` et `isAllowedDemuxer` ; échouer (et non noter) si le module existe sans eux | devops (+ secdevops relit) | ENH-008 (avant ou avec le merge) |
| SEC-25 | LOW | Élevée (constaté) | `docs/qa/resultats/2026-10-23_ENH-008.md` (non commité) | Le rapport QA énumère les logiciels ouverts sur la machine du PO (navigateur, utilitaires constructeur, client cloud) : inventaire de la machine personnelle, inutile à la mesure | Remplacer par « applications de bureau ouvertes » avant le commit ; règle QA : ne citer ni logiciels personnels, ni comptes, ni chemins du profil | qa-tester | ENH-008 |

Les MEDIUM et LOW sont proposés au scrum-master comme items de backlog (ou critères d'acceptation ajoutés aux stories citées).

## 5. Fuzzing du démux et du décodage (ENH-015)

Objectif : prouver que la **couche d'intégration** (AVIO maison, limites, conversions d'erreurs, calculs de tailles) ne plante pas et ne fuit pas sur des entrées hostiles. FFmpeg lui-même est fuzzé en continu en amont (OSS-Fuzz) ; on ne refait pas ce travail.

1. **Corpus de départ** (généré par script, jamais commité s'il contient des clips ; seuls les générateurs sont versionnés) : MP4/MOV, MKV/WebM, AVI, MPEG-TS, FLV ; H.264, HEVC 8/10 bits, AV1, VP9, MPEG-2 entrelacé ; rotation, SAR, VFR ; sous-titres SRT/ASS/PGS ; pièces jointes ; plus les cas forgés : fichier tronqué à 1, 10, 50, 99 % ; dimensions 1×1, 8193×8193, 65535×65535 ; 1000 pistes ; durée de 10 ans ; horodatages négatifs ou décroissants ; MOV avec `dref` externe ; liste HLS renommée `.mp4` ; script `concat` ; fichier de 0 octet ; fichier verrouillé ; lien symbolique ; chemin de 400 caractères ; nom Unicode.
2. **Mutation** : harnais C++ `tests/fuzz/probe_fuzz.cpp` qui appelle `IMediaProbe::probe` puis décode N images (repli logiciel, sans GPU) sur des mutations (inversion de bits, octets aléatoires, troncature, duplication de blocs) ; MSVC prend en charge libFuzzer (`/fsanitize=fuzzer,address`) **[H]** à valider par devops ; à défaut, boucle de mutation déterministe (graine fixée) de 10 000 itérations.
3. **Critères** : aucun plantage, aucune fuite (ASan), toute erreur = `PipelineError` attendu, temps par fichier ≤ 2 s pour le sondage ; tout plantage reproduit → test de non-régression.
4. **Contrôle réseau pendant le fuzzing** : les cas « référence externe » ne doivent produire **aucune** ouverture hors du fichier testé (journal de l'`io_open` maison) ni connexion (voir `zero-reseau.md` M1).
5. CI : 2 minutes de fuzzing par exécution sur le build sans GPU ; campagne longue (1 h) en local avant chaque release.

## 6. Risques résiduels acceptés (à valider par le PO)

| # | Risque | Pourquoi on l'accepte | Revue |
|---|---|---|---|
| R-1 | Une faille inconnue de FFmpeg peut donner l'exécution de code via un fichier piégé, tant que le sondage tourne dans le processus principal | Mêmes risques que tout lecteur vidéo ; mesures §3.1 ; isolement en processus prévu (SEC-09) | Chaque release |
| R-2 | Le pilote NVIDIA (NGX Core, services NVIDIA) peut communiquer avec NVIDIA indépendamment d'Enhance | Hors de notre contrôle ; on prouve qu'**Enhance** n'émet rien et que l'app fonctionne hors ligne | ENH-028 |
| R-3 | Un attaquant ayant les droits de l'utilisateur peut modifier le dossier de l'application ou `%LOCALAPPDATA%\Enhance` | Il a déjà le contrôle de la session ; on protège seulement contre la corruption et les effets de bord | — |
| R-4 | Accès SMB aux partages choisis par l'utilisateur | Fonction demandée (ADR-010) ; trafic de Windows, à la demande | ENH-028 |
| R-5 | Binaires non signés jusqu'à la diffusion publique | Démo au PO uniquement | ADR-034 |
| R-6 | Brevets vidéo (HEVC, AV1) : les licences NVIDIA laissent cette responsabilité au développeur (RTX supplément art. 5, DesignWorks art. 7, CUDA art. 2.4) | Encodage matériel NVENC, publication gratuite ; je ne suis pas juriste | Avant diffusion publique |

## 7. Exigences à transformer en stories ou critères d'acceptation

- **ENH-001 / ENH-028** : test dynamique NGX hors ligne (SEC-01, SEC-07) ; DLL `rel` uniquement, chargement par chemin absolu.
- **ENH-002** : interpolation livrable = RIFE 4.25 par **export ONNX maison** depuis les poids MIT (SEC-03) ; FRUC = repli local non livré (SEC-04) ; l'ONNX vs-mlrt reste local ; TensorRT (SEC-02, levé le 2026-10-21) : usage local et livraison OK sous conditions ; le spike peut avancer en local.
- **ENH-003** : jobs CI de `ci-securite.md`, hook de pré-commit, `.gitleaks.toml`, SEC-05, SEC-17.
- **ENH-004** : durcissement du chargement de DLL (SEC-08), manifeste `asInvoker` + `longPathAware` (SEC-20).
- **ENH-015** : limites §3.1, fuzzing §5, SEC-13 (ENH-008 a posé `checkedMul` / `checkDimensions` 1..8192 et `av_image_check_size2` ; restent : minimum 16 px, pixels ≤ 8192×4320, `max_streams`, `probesize`, `analyzeduration`, `av_max_alloc`, échéance de sondage, cas « référence externe » du fuzzing), SEC-23 ; build FFmpeg minimal préparé (SEC-06) avec exactement les démuxeurs de la liste blanche d'ENH-008.
- **ENH-022 / ENH-024** : SEC-10, SEC-11, SEC-18.
- **ENH-025** : SEC-14.
- **ENH-030** : SEC-06 (porte), SEC-16, SEC-21, contrôle `check-no-network.ps1` sur le dossier livré, `THIRD-PARTY-LICENSES.txt`.
- **Nouvelle story (plus tard)** : sondage isolé dans un processus restreint (SEC-09).
