# Threat model — § 5-7 Fuzzing, risques résiduels, exigences

> Suite de [threat-model.md](threat-model.md) — extrait du fichier d'origine lors du ménage docs du 2026-10-29, texte inchangé.

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
| R-7 | **DLL de fonction NGX vérifiée après son chargement** (détection, pas prévention) : NGX choisit et initialise lui-même la DLL (éventuellement une copie rangée dans `%ProgramData%\NVIDIA\NGX\models`, dossier **inscriptible par tous les utilisateurs**, constaté le 2026-10-24) avant qu'Enhance puisse la contrôler ; un refus n'annule pas ce qui a déjà été exécuté | Décision PO du 2026-10-24 (règle de signature d'ENH-040) ; NGX annonce un chargement « sécurisé » (`NGXSecureLoadFeature` dans son journal) **[H : non vérifié par nous]** ; même exposition pour toute application NGX du poste (jeux DLSS) ; prévention étudiée en ENH-041 ; constat à ajouter au signalement NVIDIA (§ 12 d'ENH-040) | ENH-041, chaque version du pilote |
| R-8 | **Télémétrie d'initialisation de NGX** (`NGXSendTelemetryInitializeData` : nom de l'exe, versions, identifiants de la carte d'après le journal NGX ; aucune donnée de fichier vue) | Variante de R-2 si elle passe par le pilote (`_nvngx.dll` n'importe aucune DLL réseau ; fonctions `NGXNVAPITelemetryInit` : passage par NVAPI probable **[H]**) et non par une connexion de `enhance.exe` : à **mesurer** en ENH-041 ; si `enhance.exe` émet lui-même du trafic, SEC-01 repasse HIGH et le PO est saisi | ENH-041 |

## 7. Exigences à transformer en stories ou critères d'acceptation

- **ENH-001 / ENH-028** : test dynamique NGX hors ligne (SEC-01, SEC-07) ; DLL `rel` uniquement, chargement par chemin absolu.
- **ENH-041** (preuve dynamique NGX, après ENH-040) : M1–M3 de `zero-reseau.md` sur une **session complète** (init NGX, création de l'effet, évaluation, arrêt), pas seulement l'init : (a) destination et canal de `NGXSendTelemetryInitializeData` (émis par `enhance.exe` ou par un service du pilote ?) ; (b) aucun `nvngx_update.exe` ni autre processus enfant hors de la fenêtre bloquée ; (c) `WinVerifyTrust` hors ligne (pare-feu fermé, cache CRL vidé) : aucune connexion de `enhance.exe`, verdict inchangé (SEC-34) ; (d) SEC-30 (échec du Job Object) et SEC-31 (magasin NGX inscriptible) traités ; (e) prévention : contrôle du magasin NGX **avant** `rvb_vsr_create` ; (f) liste exacte des 27 exports NGX figée dans le test (SEC-32).
- **ENH-002** : interpolation livrable = RIFE 4.25 par **export ONNX maison** depuis les poids MIT (SEC-03) ; FRUC = repli local non livré (SEC-04) ; l'ONNX vs-mlrt reste local ; TensorRT (SEC-02, levé le 2026-10-21) : usage local et livraison OK sous conditions ; le spike peut avancer en local.
- **ENH-003** : jobs CI de `ci-securite.md`, hook de pré-commit, `.gitleaks.toml`, SEC-05, SEC-17.
- **ENH-004** : durcissement du chargement de DLL (SEC-08), manifeste `asInvoker` + `longPathAware` (SEC-20).
- **ENH-015** : limites §3.1, fuzzing §5, SEC-13 (ENH-008 a posé `checkedMul` / `checkDimensions` 1..8192 et `av_image_check_size2` ; restent : minimum 16 px, pixels ≤ 8192×4320, `max_streams`, `probesize`, `analyzeduration`, `av_max_alloc`, échéance de sondage, cas « référence externe » du fuzzing), SEC-23 ; build FFmpeg minimal préparé (SEC-06) avec exactement les démuxeurs de la liste blanche d'ENH-008.
- **ENH-022 / ENH-024** : SEC-10, SEC-11, SEC-18.
- **ENH-025** : SEC-14.
- **ENH-030** : SEC-06 (porte), SEC-16, SEC-21, contrôle `check-no-network.ps1` sur le dossier livré, `THIRD-PARTY-LICENSES.txt`.
- **Nouvelle story (plus tard)** : sondage isolé dans un processus restreint (SEC-09).
