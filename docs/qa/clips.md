# Enhance — Clips de référence (ENH-005)

Statut : **v0.1 proposée**, 2026-10-21 (qa-tester). **Rien n'est téléchargé à ce stade** : le PO valide la liste, puis `scripts/fetch-clips.ps1` récupère les sources dans `tests/clips/` (ignoré par git).
Les clips ne sont **jamais commités** ; seuls ce document et la table `tests/quality/clips.psd1` (URL, noms, instants d'extrait, SHA-256) sont versionnés.
Usage : tests locaux seulement, **aucune redistribution** des clips ni de leurs versions dégradées ou traitées (les licences « NC-ND » éventuelles sont donc respectées).

**Limites honnêtes de cette liste (v0.1)** : les URL exactes ci-dessous sont les emplacements officiels *attendus*, écrits de mémoire sans accès réseau ; elles sont à confirmer à la première récupération (le script s'arrête proprement sur une 404). Les licences sont celles annoncées par les projets ; **secdevops les confirme** avant le premier téléchargement. Les tailles sont des ordres de grandeur. Les instants de début d'extrait sont des valeurs de départ à fixer après visionnage.

## 0. Première vague réellement récupérée (2026-10-21, accord du PO)

Seules BBB4K60 et TOS4K ont été téléchargées (+ SYN-TEXT généré). **Les URL de la v0.1 étaient fausses** : les deux fichiers sont servis **zippés** ; `clips.psd1` et `fetch-clips.ps1` gèrent maintenant l'archive (SHA-256 = celui de l'archive telle que servie, extraction du seul membre attendu).

| Source | URL confirmée (200 OK) | Taille archive | SHA-256 de l'archive | Fichier extrait | Caractéristiques réelles |
|---|---|---|---|---|---|
| BBB4K60 | https://download.blender.org/demo/movies/BBB/bbb_sunflower_2160p_60fps_normal.mp4.zip | 671 868 845 o | `8f1d53cf56a314be08206ae730546ef26db9fd0114b33ff2528f522bc3714d19` | `bbb_sunflower_2160p_60fps_normal.mp4` (673 223 862 o, SHA-256 `35db9a007021f1b0066993e1d2c4448c83a8b279f799c97d33cbba73980a8a36`) | H.264 3840×2160, 60 i/s, yuv420p, non étiqueté couleur (BT.709 supposé), 634,5 s, audio mp3+ac3 |
| TOS4K | https://download.blender.org/demo/movies/ToS/tearsofsteel_4k.mov.zip | 6 737 593 114 o | `9c0b8df301d254e83d0046c530a71438bbfc42d2b7000366edde1596616bad05` | `tearsofsteel_4k.mov` (6 737 592 810 o, SHA-256 `89b7fd21c7729b7d5071af993939997f847b5af06613a3388ba158dae9e52ab3`) | H.264 High **3840×1714** (pas 4096), 24 i/s, yuv420p, non étiqueté couleur, 734 s |

Licence : CC BY 3.0 (© Blender Foundation). Aucune redistribution.

**Ratio 2,39:1** : pour TOS, la vérité terrain est **3840×1710** (recadrage de 2 lignes en haut et en bas, multiple de 6 pour que le 720p fasse 1280×570), **sans bandes noires** (des bandes identiques gonfleraient les scores). La sortie d'Enhance sur ces clips doit donc faire 3840×1710 ; `Measure-Quality.ps1` compare les dimensions à celles de la vérité terrain.

**Extraits fixés après visionnage** (10 s chacun, `Start` dans `clips.psd1`) :

| ID | Source, début | Contenu réel | Mode | SHA-256 de `ref4k/<ID>.mkv` | SHA-256 de `in720/<ID>.mp4` (jeu B) |
|---|---|---|---|---|---|
| BBB-FAST | BBB4K60, 408 s | écureuil volant en plein vol, plan du lapin, glissade (mouvement rapide, 3 coupures de plan) | Interp60 | `bd2690d66a8a2c1d8696acb1ae080978516550ac9a3f3ee15631c2f7e2b87c60` | `fecbbaf0a33f2f7dae92ceae0db1fa8c10a20a2c82f08c6ee1b897e72196a7bc` |
| BBB-PAN | BBB4K60, 5 s | panoramique continu sur nuages, arbres, prairie (sans coupure) | Interp60 | `635a94400cfac7b61471eebcb7bfbacccc762e04225d713fdc1449ac270fba9f` | `39840880775a22b4c0cafe61979a71e8952270e6e6dd7f6dc78089e91351ad07` |
| TOS-FACE | TOS4K, 316 s | gros plans de visages en champ-contrechamp, faible profondeur de champ (une incrustation graphique rose vers 6-8 s) | Align24 | `c88a95852bdbd4f98b437a2a9100a9e3ac86e582b46e090736660ea8f82f9116` | `1145900b3d24e6850457fa3a26e2b05f9034a9a83d2c325b811ecca66984896e` |
| TOS-DARK | TOS4K, 153 s | cathédrale sombre et brumeuse, personnage qui descend à la corde (3 s d'un plan de visage en début d'extrait non inclus) | Align24 | `6839b72de209a768f98b74bc68f5801888e4a11abbccb262b5226c788f846bd2` | `0889c1a1024de9156a762abcba232d08ae46ad695c8d31ddc5b888421b28ec95` |
| SYN-TEXT | généré (Segoe UI), 60 i/s | texte fin défilant (26 px et 12 px) + grille de lignes de 1 px | Full | `0dfb54cce8e87848f78b01807dc9e6aefe97dfc6c212662a669b8b0939d66f0e` | `049571d02b59d22ff32499fba3ff3b5bdaad3c3beb9930e8cbe3ba0e95c098cd` |

Les sommes des fichiers FFV1/H.264 produits dépendent de la version de FFmpeg (voir `docs/qa/outils.md`) : elles servent à détecter un changement de génération, pas à vérifier un téléchargement. Espace disque réel : sources ≈ 7,4 Go (+ 7,4 Go d'archives zip, supprimables), `ref4k` ≈ 3,6 Go, `in720` ≈ 0,5 Go (jeu A) + 0,1 Go (jeu B).
CROWD-50, SOL-2D, MERI-GRAIN, PO-RETRO, PO-ARCH : **non récupérés** (2e vague, en attente du PO).
## 1. Sources (7 fichiers) → 8 extraits libres

Le script travaille sur des **sources** (téléchargées une fois), dont on tire des **extraits** de 10 s.

| Source | Où | Licence | Définition / cadence d'origine | Taille approx. |
|---|---|---|---|---|
| **BBB4K60** — *Big Buck Bunny*, `bbb_sunflower_2160p_60fps_normal.mp4` | https://download.blender.org/demo/movies/BBB/ (page : https://peach.blender.org/download/) | CC BY 3.0 (© Blender Foundation, www.bigbuckbunny.org) | 3840×2160, **60 i/s**, H.264 (déjà compressé) | ≈ 0,35 Go |
| **TOS4K** — *Tears of Steel*, `tearsofsteel_4k.mov` | https://download.blender.org/demo/movies/ToS/ (page : https://mango.blender.org/download/) | CC BY 3.0 (© Blender Foundation, mango.blender.org) | 4096×1714 ou 3840×1600 selon la version (film 2,39:1), **24 i/s** | ≈ 1,5 à 2,5 Go **[à confirmer]** |
| **CROWD50** — *Crowd Run* (séquence SVT « multiformat »), `crowd_run_2160p50.y4m` | https://media.xiph.org/video/derf/ (dossier `y4m/`) | CC BY 4.0 (© SVT, Sveriges Television) **[à confirmer par secdevops]** | 3840×2160, **50 i/s**, y4m 4:2:0 non compressé | ≈ 6 Go (le fichier complet fait ≈ 10 s) |
| **SOL4K** — *Sol Levante* (Netflix Open Content, animation 2D) | https://opencontent.netflix.com/ | Netflix Open Content License / CC BY **[à confirmer, variable selon l'œuvre]** | 4096×2160, **24 i/s**, anime 2D, grands aplats et dégradés | ≈ 1 à 5 Go **[à confirmer ; ProRes serait bien plus gros : prendre la version la plus légère]** |
| **MERI4K** — *Meridian* (Netflix Open Content, court métrage) | https://opencontent.netflix.com/ | CC BY-NC-ND 4.0 **[à confirmer]** (OK : usage privé, sans redistribution) | 3840×2160, **≈ 24 i/s**, grain de film, scènes sombres ; **version SDR obligatoire** (la version HDR PQ est écartée : HDR hors MVP) | ≈ 1 à 5 Go **[à confirmer]** |
| *(aucun téléchargement)* **SYN-TEXT** — généré par FFmpeg | — | Aucune (contenu synthétique généré localement) | 3840×2160, **60 i/s** | 0 (≈ 1 Go de disque après extraction) |

**Total à télécharger : ≈ 9 à 15 Go** (BBB + TOS + CROWD = ≈ 8 à 9 Go incontournables ; SOL et MERI ajoutent 2 à 10 Go selon les versions disponibles). **Espace disque de travail : ≈ 20 Go** (vérités terrain FFV1 sans perte ≈ 1,5 à 3,5 Go par extrait de 10 s en 4K).
**Réduire le coût si le PO le souhaite** : phase 1 = BBB4K60 + TOS4K seulement (≈ 2 à 3 Go) → 4 extraits (BBB-FAST, BBB-PAN, TOS-FACE, TOS-DARK) + SYN-TEXT = déjà 5 clips couvrant interpolation, visage, sombre, 24 i/s, texte fin ; phase 2 = CROWD, SOL, MERI à l'étape suivante.

## 2. Extraits utilisés (10 clips, dont 2 archives du PO)

Durées de 10 s pour les clips à vérité terrain (≈ 240 à 600 images) ; 15 s pour les archives. Les débuts sont des **valeurs de départ [H]** (`Start` dans `clips.psd1`), à ajuster après visionnage.

| ID | Source | Début (s) / durée | Cas couvert | Mode (protocole §3.2) | SHA-256 de la **source** |
|---|---|---|---|---|---|
| **BBB-FAST** | BBB4K60 | 30 / 10 | Animation 3D, **mouvements rapides** (personnages, objets), fourrure et herbe fines ; **interpolation 30 → 60** avec vérité | Interp60 | `à remplir à la récupération` |
| **BBB-PAN** | BBB4K60 | 300 / 10 | **Panoramique de caméra** lent sur décor riche en détails (feuillages) : tient-on le flou de bougé, l'écoulement du mouvement ; interpolation | Interp60 | idem |
| **TOS-FACE** | TOS4K | 420 / 10 | **Visage / peau** en prise de vue réelle, gros plans ; **24 i/s cinéma** | Align24 | `à remplir` |
| **TOS-DARK** | TOS4K | 60 / 10 | **Scène sombre** avec bruit et gradients (risque de bandes, de bruit amplifié) ; 24 i/s | Align24 | idem |
| **CROWD-50** | CROWD50 | 0 / 10 | **Foule et textures fines**, mouvements rapides ; **50 i/s : pas d'interpolation** (test de la règle ≥ 50 i/s) | NoInterp50 | `à remplir` |
| **SOL-2D** | SOL4K | 60 / 10 | **Animation 2D** : aplats, contours nets, dégradés (bandes, halos) ; 24 i/s | Align24 | `à remplir` |
| **MERI-GRAIN** | MERI4K | 120 / 10 | **Grain de film** et noir profond (le réseau doit ne pas lisser le grain ni l'inventer) ; 24 i/s | Align24 | `à remplir` |
| **SYN-TEXT** | généré | 0 / 10 | **Texte fin et lignes de 1 px** (défilement, grilles) : proxy des interfaces et textes de jeux rétro, tests de netteté sans licence | Full | sans objet (déterministe ; la somme du fichier produit est consignée à la génération) |
| **PO-RETRO** | archive du PO | 15 s | **Capture de jeu rétro** (pixel art, texte d'interface, 240p–720p) : le cas d'usage du joueur rétro ; **pas de vérité 4K** | NoRef | sans objet |
| **PO-ARCH** | archive du PO | 15 s | **Archive familiale SD / entrelacée** (DV, DVD, caméscope) ; bruit, désentrelacement, ratio 4:3 | NoRef | sans objet |

Couverture demandée : visage/peau (TOS-FACE) ; texte/UI de jeu rétro (SYN-TEXT + PO-RETRO) ; grain (MERI-GRAIN) ; mouvements rapides et panoramiques (BBB-FAST, BBB-PAN, CROWD-50) ; scène sombre (TOS-DARK, MERI-GRAIN) ; animation 2D (SOL-2D) ; 24 i/s cinéma (TOS-*, SOL, MERI) ; entrelacé / archive SD (PO-ARCH, seul cas disponible : aucune source entrelacée libre 4K n'existe, et la vérité terrain d'une source SD est de toute façon impossible).
Sources déjà compressées (BBB4K60 en H.264, TOS) : la « vérité terrain » contient déjà la perte de compression de la source ; c'est la même pour toutes les références comparées et on ne peut pas faire mieux avec du contenu libre. À noter dans chaque rapport.

## 3. Extraits d'archives du PO (jamais commités)

- 1 ou 2 extraits (PO-RETRO, PO-ARCH), choisis par le PO, **copiés** dans `tests/clips/po/` (dossier ignoré par git), avec le nom `PO-RETRO.<ext>` et `PO-ARCH.<ext>`. L'extrait est décrit dans ce document **sans chemin, sans nom de fichier d'origine, sans date ni nom de personne** : seulement format, définition, cadence et durée. Fiche à remplir par le PO (3 lignes) :
  - PO-RETRO : plateforme / jeu *(facultatif)*, définition d'origine, cadence, durée. `à renseigner`
  - PO-ARCH : support (DV, DVD, téléphone ancien…), définition, entrelacé oui/non, cadence, durée. `à renseigner`
- Ils ne sont **pas** hachés dans le dépôt ; leur SHA-256 reste dans `tests/clips/po/SHA256.txt` (ignoré), pour s'assurer qu'on rejoue toujours le même extrait.
- Pas de vérité 4K : ils sont jugés par conformité, vitesse, VRAM, cas limites et comparaison visuelle à l'aveugle avec Broadcast (`docs/qa/broadcast.md`).
- Le PO peut demander leur suppression à tout moment ; aucune copie ailleurs que dans `tests/clips/po/`.

## 4. Arborescence locale (tout est ignoré par git)

```
tests/clips/
  src/<fichier source>             téléchargements bruts (vérifiés par SHA-256)
  ref4k/<ID>.mkv                   vérité terrain, 4K, FFV1 sans perte (extrait 10 s)
  in720/<ID>.mkv                   jeu A : 720p sans perte (entrée d'Enhance et de Lanczos)
  in720/<ID>.mp4                   jeu B : 720p H.264 8 bits CRF 12 (entrée de Broadcast, d'Enhance et de Lanczos pour la comparaison)
  po/PO-RETRO.*  PO-ARCH.*         archives du PO (copiées à la main)
  out/<preset>/<ID>.mp4            sorties d'Enhance (produites par les tests)
  broadcast/<ID>.mp4               sorties de NVIDIA Broadcast (produites par le PO)
  python/<ID>.mp4                  sorties de l'ancien pipeline Python (facultatif, PO)
```

## 5. Récupération

`scripts/fetch-clips.ps1 -FfmpegDir <dossier du FFmpeg de mesure>` (voir l'en-tête du script) : télécharge en HTTPS (hôtes autorisés listés dans `clips.psd1`), **vérifie la somme SHA-256** (à la première récupération elle est seulement affichée : on la reporte ici et dans `clips.psd1` après relecture), découpe les extraits, génère `ref4k`, `in720` (jeux A et B) et SYN-TEXT. Le script **n'envoie aucune donnée**, n'ajoute aucun identifiant d'URL, et n'est jamais lancé par la CI.

