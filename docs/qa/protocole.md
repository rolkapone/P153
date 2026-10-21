# Enhance — Protocole qualité (ENH-005, base d'ENH-029)

Auteur : qa-tester. Statut : **v0.1 proposée** le 2026-10-21, à valider par le PO avec la liste de téléchargements. Les seuils marqués **[H]** sont des hypothèses, à confirmer ou réviser après les spikes ENH-001/002 (première mesure réelle).
Documents liés : `docs/qa/clips.md` (clips), `docs/qa/broadcast.md` (ce que le PO produit avec Broadcast), `tests/quality/` (scripts), `docs/vision.md` §5 (objectifs), `docs/gpu/architecture-pipeline.md` §4 (préréglages), `docs/architecture.md` ADR-031 (`scripts/test-gpu.ps1`).

## 1. Principe

On ne juge pas « à l'œil » : on part d'une **vérité terrain 4K réelle**, on la dégrade comme le ferait la vie (720p), on traite avec Enhance, puis on compare le résultat à la vérité d'origine. Plus le résultat est proche du 4K d'origine, meilleur est l'upscaling. Les mêmes entrées passent par **trois références à battre** :

| Référence | Rôle | Qui la produit |
|---|---|---|
| **Lanczos** (FFmpeg `scale=…:flags=lanczos`, cadence par duplication `fps=60`) | Plancher : ce que fait n'importe quel lecteur. Enhance doit le dépasser nettement. | Scripts `tests/quality/` (à la volée, sans fichier) |
| **NVIDIA Broadcast 2.3** | Référence à égaler (vision §5). Pas d'API : le PO la produit à la main. | PO (voir `docs/qa/broadcast.md`) |
| **Ancien pipeline Python du PO** (Real-ESRGAN + RIFE) | Point de comparaison facultatif, si disponible. Pas de seuil bloquant : information. | PO |

## 2. Chaîne de mesure

### 2.1 Outils (épinglés et notés dans chaque rapport)

- **FFmpeg « outil de mesure » GPL avec `libvmaf`**, hors dépôt (ADR-025 : `C:\Tools\ffmpeg\`), chemin passé en paramètre `-FfmpegDir`. Il n'est ni lié, ni livré, ni utilisé par l'application. Version de FFmpeg, version de libvmaf et nom du modèle sont **écrits dans chaque JSON de résultat** (`tool.ffmpeg`, `tool.libvmaf`, `tool.model`). Une comparaison entre deux rapports n'a de sens que si ces trois valeurs sont identiques ; sinon le rapport le signale en tête et on refait la mesure de référence de `main`.
- Syntaxe attendue : FFmpeg 6.x ou plus récent avec libvmaf 2.x (`libvmaf=model=version=…:feature=name=psnr|name=float_ssim`).
- Les scripts n'ont pas besoin de Python (PowerShell 7 seulement). Si un jour Python devient nécessaire (ex. tracés), il sera lancé avec `-I` dans un venv dédié.

### 2.2 Métriques

| Métrique | Définition retenue | Pourquoi |
|---|---|---|
| **VMAF** | modèle **`vmaf_4k_v0.6.1`** (affichage 4K, distance 1,5 × hauteur), moyenne arithmétique sur les images mesurées ; on consigne aussi la moyenne harmonique, le minimum et le **1er centile** par image. | Corrèle le mieux avec la perception sur du 4K. |
| **VMAF NEG** | modèle **`vmaf_4k_v0.6.1neg`** (« no enhancement gain »), mesuré en second passage. **Garde-fou, pas métrique principale.** | Un réseau de super-résolution peut sur-accentuer et « gonfler » le VMAF standard ; le NEG neutralise ce gain. Si VMAF monte mais NEG ne monte pas, c'est de la netteté artificielle, pas de la fidélité. |
| **SSIM** | SSIM de la luminance seule (Y), calculé par libvmaf (`float_ssim`), moyenne sur les images. | Structure ; ne dépend pas d'un modèle appris. |
| **PSNR Y** | `psnr_y` de libvmaf en dB, sur la luminance seule, calcul par image puis moyenne (pas PSNR global). Plafond à 60 dB par image si l'image est identique. | Fidélité pixel ; fonction connue de tous. |

Aucune métrique n'est utilisée seule : un résultat « meilleur » exige que **les trois** s'améliorent (voir les seuils, §7). Les scores sont **toujours rapportés avec leur écart à Lanczos** (mêmes images, même clip).

### 2.3 Normalisation avant comparaison

Pour que l'écart mesuré vienne de l'upscaling et pas d'un détail de format :

1. **Même résolution** : la sortie d'Enhance doit faire exactement 3840×2160 (sinon le script refuse de comparer et le note : c'est aussi un défaut de conformité, §6).
2. **Espace couleur et plage** : MVP = SDR BT.709, plage limitée (`tv`). Les deux flux sont ramenés par `format=yuv420p10le` **sans conversion de matrice ni de plage** (les deux sont déjà BT.709 limité). Si la sortie est étiquetée autrement (ex. plage pleine, BT.601), la comparaison est invalide : bug bloquant de conformité, on ne « corrige » pas côté mesure. Contrôle complémentaire : écart des moyennes Y/U/V entre sortie et référence (`signalstats`) ≤ 1 niveau sur 1023 ; au-delà, un décalage de teinte ou de plage est signalé.
3. **8 bits contre 10 bits** : tout est comparé en 10 bits (le 8 bits est étendu sans arrondi par `format`). La vérité terrain 8 bits ne profite pas de la sortie 10 bits ; le gain du 10 bits (moins de bandes sur les dégradés) est donc **mesuré à part** sur les clips à dégradés (SOL-2D, MERI-GRAIN, TOS-DARK) : on encode une fois en 8 bits et une fois en 10 bits, mêmes préréglages, et on compare VMAF/PSNR et le nombre de zones en bandes à l'œil. Pas de seuil bloquant au MVP, information.
4. **Alignement temporel** (§3.3) : mêmes images comparées, vérifié avant chaque mesure.
5. **Encodage de la sortie** : on mesure **le fichier réellement produit** (HEVC / AV1 décodé par FFmpeg), pas une sortie intermédiaire. La mesure inclut donc la perte d'encodage, ce qui est voulu (c'est ce que verra l'utilisateur). Les débits sont ceux du préréglage ; pour comparer HEVC et AV1 « à débit égal » (ENH-020), on fixe le même débit moyen aux deux.

## 3. Méthode de référence

### 3.1 Génération des entrées (script `scripts/fetch-clips.ps1`, voir `docs/qa/clips.md`)

Pour chaque clip :

1. `ref4k/<ID>.mkv` : extrait de 10 à 20 s de la **source 4K** (jamais de recompression avec perte), en **FFV1 sans perte**, au format et à la cadence d'origine. C'est la **vérité terrain**.
2. `in720/<ID>.mkv` (**jeu A**, sans perte, 8 ou 10 bits comme la source) : `scale=1280:720:flags=lanczos` puis réduction de cadence si prévue (voir 3.2). Entrée d'Enhance et de la ligne Lanczos.
3. `in720/<ID>.mp4` (**jeu B**) : même contenu en H.264 8 bits, CRF 12, étiqueté BT.709 limité. C'est ce qu'on donne à **NVIDIA Broadcast** (qui n'accepte pas du FFV1). Pour comparer équitablement, **Enhance et Lanczos sont aussi mesurés sur le jeu B** dans les comparaisons avec Broadcast. Les seuils vs Lanczos (§7) se jugent sur le jeu A.

La réduction 4K → 720p utilise **toujours le même filtre Lanczos** : un flou de réduction plus doux que celui du monde réel ne change pas le classement, mais le chiffre absolu de VMAF, d'où l'importance de comparer à Lanczos et jamais « dans l'absolu ».

### 3.2 Cadence : trois cas

Cadence de sortie d'Enhance (décision PO n° 1) : source ≥ 50 i/s conservée ; source < 50 i/s portée à 60 i/s.

| Cas | Source 4K | Entrée 720p | Sortie attendue | Images comparées |
|---|---|---|---|---|
| **Interp60** (BBB-FAST, BBB-PAN) | 60 i/s | 30 i/s : on **retire une image sur deux** (`select='not(mod(n,2))'`, sans mélange) | 60 i/s | **`Kept`** : images paires (présentes en entrée, mesure de la super-résolution seule) ; **`Generated`** : images impaires (créées par l'interpolation, comparées à l'image retirée). L'interpolation se juge **uniquement sur `Generated`**. |
| **NoInterp50** (CROWD-50) | 50 i/s | 50 i/s (aucune réduction de cadence) | 50 i/s, horodatages conservés | `All` |
| **Align24** (TOS-FACE, TOS-DARK, SOL-2D, MERI-GRAIN) | 24 i/s | 24 i/s | 60 i/s | Pas de vérité à 60 i/s : on compare seulement les images **alignées dans le temps**, soit 1 image de sortie sur 5 (sortie `n % 5 == 0` ↔ source `n % 2 == 0`, car 5/60 s = 2/24 s). Les 4 autres images sur 5 sont interpolées sans vérité : jugées par l'œil et par les contrôles de conformité (régularité des horodatages). |
| **Full** (SYN-TEXT) | 60 i/s synthétique | 60 i/s | 60 i/s | `All` ; clip volontairement non interpolé pour isoler la super-résolution sur du texte fin. |

### 3.3 Alignement temporel (contrôle automatique avant toute mesure)

Un décalage d'une seule image fait chuter le VMAF de plusieurs points et fausserait toute conclusion. Avant chaque comparaison, `Measure-Quality.ps1` :

1. vérifie que premier horodatage = 0 (± 1 ms) des deux flux et que le nombre d'images de la sortie est celui attendu **± 1** (la dernière image interpolée peut manquer) ; il compare alors sur `min(images)` ;
2. calcule le PSNR Y sur 20 images pour des décalages de −1, 0 et +1 image ; **le meilleur doit être le décalage 0** ; sinon la mesure est marquée `ALIGNEMENT_SUSPECT` et rejetée (bug à signaler, pas à corriger côté mesure).

### 3.4 Qualité d'une exécution Lanczos (plancher)

Commande équivalente (le script la génère) : entrée jeu A → `scale=3840:2160:flags=lanczos,fps=60` (ou cadence d'origine pour NoInterp50), flux envoyé directement à `libvmaf` (aucun fichier intermédiaire). **Doit tenir en moins de 5 min par clip** sur la machine du PO (ENH-005) ; on peut sous-échantillonner (`-Subsample 2`) seulement pour l'itération, jamais pour un rapport officiel.
Une variante « Lanczos + `minterpolate` (mci) » est mesurée **une fois** à titre indicatif pour situer la qualité de l'interpolation d'un outil classique ; elle n'est pas un seuil.

### 3.5 Clips sans vérité terrain (archives du PO)

PO-RETRO et PO-ARCH n'ont pas de 4K d'origine : pas de VMAF. Ils servent à : la conformité (§6), la vitesse, la VRAM, la **comparaison visuelle à l'aveugle** avec Broadcast / ancien pipeline (planche d'images fixes + séquences, A/B sans étiquette tirée au hasard, verdict du PO), et les cas limites (entrelacé, ratio d'aspect, bruit). Ils ne sont jamais commités ni décrits avec un chemin personnel.

## 4. Vitesse

- **Métrique** : images **de sortie** par seconde de bout en bout (`images_sortie / durée murale`, de l'ouverture du fichier à la fermeture du fichier produit), plus le temps par étage (décodage, super-résolution, interpolation, conversions, encodage) fourni par l'instrumentation du pipeline (`enhance-bench`, sortie JSON, voir 4.3).
- **Condition machine** (consignée dans le rapport) : RTX 5070 au repos (utilisation GPU < 5 % et pas de température de départ > 50 °C), pilote noté, mode d'alimentation « Performances élevées », aucune autre application GPU ouverte (fermer navigateur, Docker, iCUE ; ~2 Gio de VRAM sont déjà pris au repos sur cette machine, d'où la mesure en différence, §5), HAGS et version de Windows notées.
- **Protocole** : 1 passe de **chauffe** non comptée (charge des modèles, construction des moteurs TensorRT, caches) puis **5 répétitions**, **médiane** retenue, min et max affichés. Entre deux répétitions, 20 s de pause. Si l'écart (max − min) / médiane dépasse **3 %**, la mesure est **signalée instable** (cause à chercher : thermique, tâche de fond) et jamais masquée en relançant jusqu'à obtenir un beau chiffre ; le rapport garde toutes les passes.
- **Chauffe thermique** : un essai **soutenu** de 3 min en boucle (même clip répété) mesure les images/s en fin d'essai ; on rapporte le ratio « soutenu / à froid » ; sous 0,9, on l'écrit (les budgets s'appliquent à la valeur **soutenue**).
- **Conditions mesurées** : entrée 720p30 (jeu A), sortie 3840×2160 à 60 i/s, HEVC 10 bits, préréglage visé, débit du préréglage ; AV1 10 bits en seconde ligne.
- **File** : le temps total d'une file de 10 clips est consigné (suite des 10 extraits), avec une cible indicative = somme des durées / vitesse médiane + 10 % ; pas de seuil bloquant.
- **Budgets** (`docs/architecture.md` / vision §5) : Qualité max ≥ 30 i/s, Équilibré ≥ 60 i/s, Rapide ≥ 100 i/s. Verdict : voir §7.

### 4.3 Instrumentation attendue du produit (demande au gpu-engineer)

Le banc a besoin d'une cible en ligne de commande sans interface, `enhance-bench.exe` (hors livrable, construite avec `ENH_WITH_GPU=ON`) :
`enhance-bench --in <fichier> --out <fichier> --preset quality|balanced|fast --codec hevc|av1 --bits 8|10 --report <json>`
Le JSON écrit : `frames_in`, `frames_out`, `wall_s`, `fps_out`, `stages[{name, ms_total, ms_per_frame}]`, `vram_peak_mib` et `vram_rest_mib` (in-process : `cuMemGetInfo` + `QueryVideoMemoryInfo`, §5), `process_private_mib` au début et à la fin, `exit_code`. Sans cette cible, `Measure-Speed.ps1` ne peut mesurer qu'avec `nvidia-smi` global (précision moindre, signalé).

## 5. VRAM

- **Pic in-process** (référence pour le seuil) : `cuMemGetInfo` au repos puis échantillonné toutes les 100 ms + `IDXGIAdapter3::QueryVideoMemoryInfo` (budget local), fournis par `enhance-bench`. Résultat = **pic − repos** en Mio. (En mode WDDM, `nvidia-smi` ne donne pas la valeur par processus, d'où l'in-process.)
- **Contrôle externe** : `nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits -lms 200` pendant tout l'essai (`Measure-Speed.ps1` le lance et en tire le pic global − repos) ; l'écart entre les deux mesures doit rester < 10 % ; au-delà, on cherche une consommation hors processus (encodeur du pilote, Windows).
- **Fuite** : traiter **10 fichiers à la suite** (suite de 10 extraits, préréglage Équilibré, puis Qualité max), relever la VRAM résiduelle **entre deux fichiers**, au repos (2 s après la fermeture de chaque fichier) ; **dérive = VRAM résiduelle après le fichier 10 − après le fichier 1**. La première passe n'est pas comparée au repos initial (chargement des modèles, caches TensorRT). Exigence DoD : dérive nulle ; **tolérance de mesure : ≤ 50 Mio** (ADR-031, bruit WDDM). On relève aussi la pente (régression linéaire des 10 points) : une pente positive régulière, même sous 50 Mio au total, est signalée comme fuite probable. Mémoire système (privé du processus) : dérive ≤ 100 Mio ; handles et threads du processus : dérive nulle.
- **Annulation** : annuler en cours de traitement, puis traiter un fichier : la VRAM doit revenir au niveau de repos de la boucle (± 50 Mio).

## 6. Conformité des fichiers produits (`Test-Conformity.ps1`, ffprobe)

À vérifier sur **chaque** fichier produit lors des essais (pas seulement les clips de qualité) :

| Contrôle | Attendu (MVP) |
|---|---|
| Conteneur | MP4 ; MKV seulement si sous-titre incompatible MP4 (message affiché, décision PO n° 2) ; `moov` en tête (démarrage rapide) |
| Codec / profil | HEVC `Main 10` (10 bits) ou `Main` (8 bits) ; AV1 `Main` ; étiquette MP4 `hvc1` / `av01` (lecture dans l'Explorateur / Films & TV) |
| Pixel | `yuv420p10le` pour 10 bits, `yuv420p` pour 8 bits |
| Résolution | 3840×2160 (ou la cible du préréglage ; dimensions paires) ; PAR 1:1 |
| Cadence | CFR : `r_frame_rate == avg_frame_rate` ; 60/1 pour une source < 50 i/s, sinon cadence et horodatages de la source |
| Nombre d'images / durée | durée vidéo = durée source **± 1 image de sortie** ; `nb_frames` = `round(durée × cadence)` ± 1 ; aucun horodatage non monotone (`pts` strictement croissants) |
| Couleur | `color_space=bt709`, `color_transfer=bt709`, `color_primaries=bt709`, `color_range=tv` (SDR) ; **HDR10 hors MVP** (quand il arrivera : `smpte2084`, `bt2020`, `mastering_display` et `content_light_level` présents) |
| Audio | même nombre de pistes, même codec ou copie, même langue ; **A/V sync** : |`start_time` audio − `start_time` vidéo| ≤ 40 ms et |durée audio − durée vidéo| ≤ 40 ms (backlog ENH, clip de 10 min) ; contrôle d'écoute sur un clip avec « clap » |
| Sous-titres | pistes conservées (nombre, langue, disposition par défaut/forcé) ; bascule MKV annoncée si PGS/VobSub/autre incompatible |
| Chapitres, métadonnées | conservés si présents ; **aucune chaîne personnelle** (pas de chemin `C:\Users\…`, pas d'e-mail) dans les tags du conteneur et des flux |
| Intégrité | `ffmpeg -v error -i fichier -f null -` : zéro erreur de décodage |

Le script sort un JSON (`checks[]` : nom, attendu, constaté, statut) et un Markdown ; un seul contrôle rouge = fichier non conforme = bug bloquant.

## 7. Seuils PASS / FAIL proposés [H]

Quatre verdicts partiels, tous requis pour un `VERDICT QA: PASS`.

### 7.1 Qualité (jeu A, moyenne sur les clips à vérité terrain, **et** minimum par clip)

Écarts **par rapport à Lanczos** mesuré sur les mêmes images :

| Préréglage | ΔVMAF 4K | ΔSSIM | ΔPSNR Y | Garde-fou NEG | Minimum par clip |
|---|---|---|---|---|---|
| **Qualité max** | ≥ +5 (vision §5) | ≥ +0,02 | ≥ +1,5 dB | ΔVMAF-NEG ≥ +3 | aucun clip sous Lanczos + 2 VMAF ; aucune métrique sous Lanczos sur aucun clip |
| **Équilibré** | ≥ +4 | ≥ +0,015 | ≥ +1,0 dB | ΔVMAF-NEG ≥ +2 | aucun clip sous Lanczos + 1 VMAF ; idem |
| **Rapide** | ≥ +2 | ≥ +0,01 | ≥ +0,5 dB | ΔVMAF-NEG ≥ +1 | aucun clip sous Lanczos ; idem |

**Interpolation** (images `Generated`, clips BBB-FAST et BBB-PAN) :

| Préréglage | VMAF `Generated` | 1er centile des images `Generated` |
|---|---|---|
| Qualité max, Équilibré | ≥ Lanczos + duplication + 3 **et** ≥ valeur du spike ENH-002 − 1 (backlog) | ≥ celui de la duplication (l'interpolation ne doit jamais être pire que répéter l'image dans ses pires cas : coupures de plan, bords) |
| Rapide (interpolation dégradée autorisée) | ≥ Lanczos + duplication + 1 | ≥ celui de la duplication − 2 |

**Contre NVIDIA Broadcast 2.3** (jeu B, au moins 2 clips ; ENH-029 exige au moins 2) : VMAF, SSIM et PSNR d'Enhance (préréglage Qualité max) **≥ ceux de Broadcast sur chaque clip**, avec **tolérance −0,5 VMAF** (Broadcast est limité à 8 bits : si Enhance est au-dessus de plus de 0,5 point, c'est un plus, pas un seuil). SSIM et PSNR : pas de tolérance. Si Broadcast n'est pas encore produit, ce seuil est **« non vérifié »** (jamais PASS par défaut) et le verdict l'écrit.
**Ancien pipeline Python** : information seulement.

### 7.2 Vitesse (images de sortie par seconde, médiane des 5 répétitions, valeur soutenue)

| Préréglage | Budget (vision §5) | PASS | PASS avec avertissement | FAIL (> 20 % sous le budget) |
|---|---|---|---|---|
| Qualité max | ≥ 30 | ≥ 30 | 24 à < 30 | < 24 |
| Équilibré | ≥ 60 | ≥ 60 | 48 à < 60 | < 48 |
| Rapide | ≥ 100 | ≥ 100 | 80 à < 100 | < 80 |

Décision PO n° 5 : si 60 i/s n'est pas tenu en Équilibré, la qualité prime et la vitesse est affichée ; l'écart est alors remonté au PO (révision du budget) plutôt que masqué. Les budgets restent ceux de `docs/architecture.md` tant qu'ils n'ont pas été révisés par ADR.

### 7.3 Ressources

| Contrôle | PASS | FAIL |
|---|---|---|
| VRAM pic (pic − repos, in-process) | ≤ 9 216 Mio (9 Gio) ; avertissement au-dessus de 8 500 Mio | > 9 216 Mio |
| Dérive VRAM sur 10 fichiers | ≤ 50 Mio, pente non positive de façon régulière | > 50 Mio, ou fuite de handles / threads |
| Dérive mémoire système sur 10 fichiers | ≤ 100 Mio | > 100 Mio |
| Plantage / blocage / sortie non nulle | aucun sur toute la matrice de cas limites (§8) | un seul |

### 7.4 Conformité et régression

- **Conformité** (§6) : 100 % des contrôles verts sur tous les fichiers produits, sinon FAIL.
- **Non-régression** : comparaison à la **dernière mesure de `main`** (même clip, même préréglage, mêmes versions d'outils). **FAIL** si, par rapport à `main` : VMAF, SSIM ou PSNR perdent plus de **5 %** (relatif), images/s perdent plus de **5 %**, ou VRAM pic gagne plus de **5 %**. Avertissement dès −1 point de VMAF, −2 % d'images/s ou +3 % de VRAM. La première mesure de `main` (baseline) sert de référence : elle est produite à la première story `[GPU]` mergée et consignée dans `docs/qa/resultats/baseline-main.json`.
- **Reproductibilité** : deux passes qui diffèrent de plus de **3 %** sont signalées (§4) ; une mesure signalée instable ne peut pas, seule, prononcer un PASS de vitesse.
- Un test qui échoue de façon aléatoire est un **bug à signaler**, jamais masqué par des relances.

## 8. Cas limites à passer (par story `[GPU]`)

Chaque cas : le traitement **ne plante pas**, produit soit un fichier conforme soit une erreur claire en français, et libère la VRAM.

1. Fichier **corrompu** (octets écrasés au milieu), **tronqué** (coupé à 60 %), **en-tête invalide**, **0 octet**.
2. **Format exotique** : AVI MPEG-4 ASP, WMV, MPEG-2 PS (DVD), DV entrelacé, ProRes, VP9/AV1 en WebM, 4:2:2 et 4:4:4, 10 bits, plage pleine, ratio d'aspect non carré (PAR ≠ 1), rotation 90° (téléphone).
3. **VFR** (téléphone) : sortie CFR conforme, synchro audio conservée.
4. **Très court** (1 s, et 1 seule image) ; **très long** (10 min pour la synchro audio, 2 h pour la VRAM / la stabilité, hors CI).
5. **Source déjà 4K** (pas de super-résolution : message `NoUpscaleAlreadyAtTarget`), source plus petite que 360p, source 50-60 i/s (pas d'interpolation).
6. **Disque de sortie plein** (volume VHD de 100 Mio) : erreur propre, fichier partiel supprimé, file continue.
7. **Nom Unicode** (accents, kanji, emoji), **chemin long** (> 260 caractères), nom avec espaces et `&`, dossier de sortie sans droit d'écriture.
8. **Annulation** à 10 %, 50 %, 99 % ; **pause / reprise** ; double-annulation ; quitter pendant un traitement (décision PO n° 6).
9. Sous-titres texte et image, plusieurs pistes audio, chapitres.
10. Fichier **ouvert par un autre programme** pendant le traitement.

## 9. Tests de l'interface (stories `[UI]`)

Si le framework de test Qt/UI Automation retenu n'automatise pas un parcours, c'est `docs/qa/checklist-ui.md` (courte, reproductible, à écrire avec la première story `[UI]`). Parcours minimal :

1. **Glisser-déposer → file → préréglage → lancement → résultat**, à la souris puis **au clavier seul** (Tab / Maj+Tab, Entrée, Espace, Échap, raccourcis ; ordre de focus logique, focus visible).
2. États : vide, survol de dépôt, 1 fichier, 10 fichiers, erreur (fichier non lisible), en cours, pause, annulé, terminé, disque plein.
3. **Mise à l'échelle Windows 100 %, 125 %, 150 %, 200 %** ; **mode sombre et mode clair** ; changement de thème à chaud.
4. **Contrastes** (≥ 4,5:1 texte, 3:1 composants), lecteur d'écran (Narrateur) sur la fenêtre principale : noms et rôles des contrôles, annonce de la progression.
5. **Fluidité pendant un traitement GPU** : l'animation d'un élément (barre de progression, survol, réordonnancement) tourne à 60 i/s ; **le fil d'interface ne reste jamais bloqué plus de 16 ms** (mesure : instrumentation du délai de la boucle d'événements dans une build de test, ou outil de capture de fréquence d'images) pendant un traitement Qualité max sur BBB-FAST. Seuil : 99e centile des images ≤ 16,7 ms (33 ms toléré sur < 0,1 % des images) ; au-delà, FAIL.
6. **Captures d'écran** de chaque état et de chaque thème / échelle, rangées dans `docs/qa/resultats/<story>/captures/` (sans chemin ni nom d'utilisateur visibles), transmises au ui-designer pour le contrôle design.
7. Textes français, pas de texte tronqué, noms de fichiers longs et Unicode lisibles.

## 10. Rapports (`docs/qa/resultats/`)

- Un rapport par exécution, nommé `AAAA-MM-JJ_HHMM_<story>_<branche>.md` + son `.json` (mêmes données) + éventuellement un `.csv` (une ligne par clip × préréglage × référence). C'est ce que `New-QualityReport.ps1` écrit ; `scripts/test-gpu.ps1` (ADR-031) écrit son propre fichier dans le même dossier pour les tests GPU, et appelle ces scripts pour les mesures rapides.
- **En-tête obligatoire** : machine (GPU, pilote), Windows, versions CUDA / SDK NVIDIA / FFmpeg de mesure / libvmaf / modèle VMAF, hash git, préréglage et codec, conditions (carte au repos, température de départ), **aucun chemin personnel, aucun e-mail**.
- **Tableau principal** (format du verdict QA) : `Clip | Préréglage | Référence | VMAF | VMAF-NEG | SSIM | PSNR Y | ΔVMAF vs Lanczos | vs Broadcast | images/s (médiane, min-max) | VRAM pic`.
- Puis : tableau de conformité (un fichier par ligne), résultats de la boucle de 10 fichiers (VRAM, mémoire, handles), cas limites (OK / KO), cases à cocher des seuils §7 (`[x]` / `[ ]` / « non vérifié »), mesures signalées instables.
- Les rapports sont versionnables mais **commités seulement sur demande** ; les clips et fichiers produits ne le sont **jamais** (`.gitignore` : `tests/clips/`, `*.mp4`, `*.mkv`).

## 11. Ce qui reste à confirmer au premier passage réel

- La séquence des instants d'extrait (`Start`) de chaque clip, à fixer après visionnage (le fichier `tests/quality/clips.psd1` les porte).
- Le coût de calcul de VMAF 4K sur la machine (objectif < 5 min par clip pour la ligne Lanczos) ; sinon sous-échantillonnage uniquement pour l'itération.
- Les budgets de vitesse et de VRAM, après ENH-001 / ENH-002 (spikes) : révision éventuelle par ADR, avec accord du PO.
- Le choix « modèle standard seul » ou « standard + NEG » comme critère officiel (proposé : standard comme critère, NEG comme garde-fou).
