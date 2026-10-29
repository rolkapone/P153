# Référence NVIDIA Broadcast 2.3.0 (build 71988672) - 2026-10-21

Mesure : jeu B (entrées 720p H.264 CRF 12) contre la vérité terrain 4K. Machine : RTX 5070, pilote 617.42, Windows 10.0.26200. FFmpeg de mesure : n8.1.3-20-gc8de997cb1 (GPL, `C:\Tools\ffmpeg`), libvmaf 86da14d, modèles `vmaf_4k_v0.6.1` et `vmaf_4k_v0.6.1neg`. Dépôt : 7f54e6c + modifications non commitées de `tests/quality/`.
Données : `2026-10-21_reference-broadcast/` (un JSON par mesure + `reference-broadcast.json` avec la conformité) et `2026-10-21_broadcast-vs-lanczos.json` (comparaison, cibles). Script : `tests/quality/Run-BroadcastReference.ps1`.

## 1. Réglages réellement utilisés (capture `reglages.png`)

Processeur de fichiers, 5 entrées de `in720` (BBB-FAST, BBB-PAN, SYN-TEXT, TOS-DARK, TOS-FACE) toutes cochées. **RTX Video Super Resolution : activé, sortie 2160p (4K)** (pas de niveau de qualité réglable dans cette version : un seul curseur, « Résolution en sortie »). **RTX Video Frame Generation : activé, « 2x »** (réglage global : appliqué à tous les fichiers, impossible de le couper par fichier). Tous les autres effets (arrière-plan virtuel, cadrage, suppression de bruit, contact visuel, vignette, lumière clé) : désactivés. Aucun réglage d'encodage visible (sortie MP4 H.264). Dossier de sortie dans `tests/clips/broadcast`. La capture ne montre aucun nom d'utilisateur.

**Mention capitale de l'interface : « L'effet est ignoré si la largeur/hauteur de sortie demandée dépasse 4096 px. »** Pour un film 2,39:1, « 2160p » donnerait 4519 × 2160 : Broadcast **n'a pas agrandi les clips TOS** (voir §2).

## 2. Conformité (ffprobe)

| Clip (fichier) | Attendu | Constaté | Verdict |
|---|---|---|---|
| BBB-FAST | 3840×2160, 60 i/s, 600 images | 3840×2160, H.264 High L5.2, yuv420p 8 bits, 60 i/s, **599 images**, 9,983 s, 49,5 Mb/s | Conforme (599 = 2×300 − 1 : la dernière image générée manque, toléré ±1) |
| BBB-PAN | idem | idem, 599 images, 64,5 Mb/s | Conforme (idem) |
| SYN-TEXT | 3840×2160, 60 i/s, 600 images | 3840×2160, 60 i/s, 600 images, 10,000 s, 23,0 Mb/s | Conforme : la génération d'images n'a **pas doublé** une source déjà à 60 i/s (pas de 120 i/s) |
| TOS-FACE | 3840×1710, 24 → 60 i/s | **1280×570, 48 i/s, 479 images**, H.264 High L3.2, 11,3 Mb/s | **Non conforme : pas de super-résolution** ; 24 → 48 i/s (2x) au lieu de 60 |
| TOS-DARK | idem | **1280×570, 48 i/s, 479 images**, 8,9 Mb/s | **Non conforme** (idem) |

Tous : premier horodatage 0, intervalles réguliers (un seul écart de 1 ms d'arrondi), aucune étiquette de couleur (espace, plage, transfert « inconnus » : à lire comme BT.709 limité, la valeur de luminance moyenne correspond à la vérité terrain à 0,3 près), pas d'audio (entrée sans audio). Pas de 10 bits ni de HDR (H.264 8 bits seulement, comme attendu).

**Conséquence pour TOS** : les deux fichiers TOS de Broadcast ne mesurent pas l'upscaling. Ils sont comparés à Lanczos tels quels (Broadcast 1280×570 passé en Lanczos vers 3840×1710 pour la mesure), ce qui mesure « ré-encodage H.264 + interpolation 24 → 48, sans super-résolution ». **Ils ne comptent pas pour le critère « Broadcast » de §7.1 du protocole** tant qu'ils ne sont pas refaits. Entrées de remplacement prêtes (rien à télécharger) : `tests/clips/in720-padded/TOS-FACE.mp4` et `TOS-DARK.mp4` (TOS 1280×570 posé sur 1280×720 avec 75 lignes noires en haut et en bas, H.264 CRF 12). Broadcast les sortira en 3840×2160 ; la mesure recadre alors la zone centrale 3840×1710 (décalage vertical 225). Le recadrage n'est pas encore branché dans le script de mesure (à faire à la réception des fichiers).

## 3. Adaptation de la mesure

- Cadence : BBB = images paires (Kept = images conservées) / impaires (Generated) du 60 i/s ; l'alignement temporel est contrôlé automatiquement (décalage 0 = meilleur, `ok` sur les 7 mesures). TOS = mode `Aligned48` (nouveau) : image 2k de la sortie 48 i/s ↔ image k de la vérité 24 i/s, 240 images ; les images impaires (générées, sans vérité) ne sont pas mesurées. SYN-TEXT = `All`.
- Comparaison avec Lanczos faite sur **les mêmes images** et le **même jeu B** (Lanczos TOS recalculé sur les 240 images à 24 i/s).
- **Bug de mesure trouvé et corrigé en cours de route** : l'appariement libvmaf d'un mp4 et d'un mkv se décalait d'une image sur trois (arrondi des horodatages). Correctif `settb=1/30,setpts=N`, toutes les mesures (Broadcast **et** Lanczos) refaites. Les premiers chiffres, abandonnés, sous-estimaient à la fois Broadcast et Lanczos.
- Reproductibilité : deux passes Lanczos sur les mêmes entrées (avant correction) ont donné des résultats identiques à la décimale ; la mesure est déterministe.

## 4. Résultats : Broadcast contre Lanczos (jeu B)

Images : Kept = conservées, Gen = générées (comparées à l'image retirée), TOS = 240 images alignées (sans super-résolution côté Broadcast).

| Clip | Images | VMAF Broadcast / Lanczos | Δ VMAF | Part de la marge | VMAF-NEG Broadcast / Lanczos | SSIM Y Broadcast / Lanczos | PSNR Y Broadcast / Lanczos |
|---|---|---|---|---|---|---|---|
| BBB-FAST | Kept | 94,70 / 90,59 | **+4,11** | 43,6 % | **78,21** / 89,05 | 0,9913 / **0,9993** | 31,24 / **42,77** |
| BBB-FAST | Gen | 89,52 / 68,63 | **+20,89** | 66,6 % | 73,59 / 66,50 | **0,9783** / 0,9529 | 30,44 / **31,51** |
| BBB-PAN | Kept | 84,12 / 81,50 | +2,62 | 14,2 % | **72,61** / 80,18 | 0,9934 / **0,9988** | 31,31 / **33,11** |
| BBB-PAN | Gen | 81,36 / 38,21 | **+43,15** | 69,8 % | 71,31 / 37,18 | **0,9936** / 0,9587 | **31,18** / 27,37 |
| SYN-TEXT | All | 68,55 / 63,23 | +5,32 | 14,5 % | 59,05 / **61,62** | 0,9804 / **0,9941** | 24,27 / **24,38** |
| TOS-FACE | 24 i/s (*) | 84,64 / 88,53 | **-3,89** | -33,9 % | 83,16 / 86,91 | 0,9934 / **0,9976** | 34,65 / **38,64** |
| TOS-DARK | 24 i/s (*) | 91,10 / 93,19 | **-2,09** | -30,7 % | 89,36 / 91,84 | 0,9947 / **0,9982** | 38,27 / **41,72** |

(*) Broadcast n'a pas agrandi TOS : non représentatif de son upscaling.

Détails complémentaires (premier centile / minimum VMAF des images, Broadcast) : BBB-FAST Kept 85,5 / 85,3 ; BBB-FAST Gen 25,2 / 6,0 (coupures de plan, comme la duplication : 13,0 / 5,6) ; BBB-PAN Kept 80,2 / 80,0 ; BBB-PAN Gen 74,0 / 52,6 ; SYN-TEXT 64,5 / 63,9.

### Lecture

- **Super-résolution (images conservées, 3 clips valides)** : Broadcast gagne en **VMAF** (+4,1 BBB-FAST, +2,6 BBB-PAN, +5,3 SYN-TEXT ; part de la marge 44 %, 14 %, 15 %), mais **perd partout en PSNR (de -0,1 à -11,5 dB) et en SSIM**, et **perd en VMAF-NEG** (-10,8, -7,6, -2,6). Le motif est celui d'une **accentuation** (ou d'un petit décalage sous-pixel) : VMAF « normal » récompense la netteté ajoutée, NEG et PSNR la pénalisent. Sur BBB-FAST, PSNR 31 dB contre 42,8 pour un simple Lanczos : Broadcast **ne reconstruit pas plus fidèlement**, il rend l'image plus piquée. À l'œil, cela peut plaire ; objectivement le gain est fragile. **Moyenne des 3 clips valides : +24 % de la marge en VMAF**, c'est-à-dire entre Rapide (20 %) et Équilibré (35 %) en VMAF, et sous Lanczos en VMAF-NEG.
- **Génération d'images** : très nette. Les images générées de Broadcast valent VMAF 89,5 (BBB-FAST) et 81,4 (BBB-PAN) contre 68,6 et 38,2 pour la duplication ; PSNR/SSIM au-dessus de Lanczos+duplication (SSIM 0,978 / 0,994 contre 0,953 / 0,959), sauf PSNR BBB-FAST (-1,1 dB, à cause des coupures de plan). **Plafond à battre pour l'interpolation d'Enhance** : 89,5 et 81,4.
- **Comparaison avec le spike du gpu-engineer (`docs/gpu/spike-phase3.md` §P3.5)** : RIFE mixte VMAF 91,5 (BBB-FAST) / 90,9 (BBB-PAN) et FRUC 89,2 / 87,8, **mais mesurés en 720p, sur l'interpolation seule, sans super-résolution ni encodage, jeu sans perte et avec le modèle par défaut** : **non directement comparables** aux 89,5 / 81,4 de Broadcast (4K de bout en bout, jeu B). Seul le classement relatif est instructif : RIFE et FRUC y dépassent la duplication de 20 à 35 points, ce qui est du même ordre que Broadcast. Pour un vrai face-à-face il faudra la mesure complète d'Enhance en 4K (étape QA des stories).
- **Texte fin (SYN-TEXT)** : Broadcast améliore peu (+5,3 VMAF, PSNR identique à Lanczos, VMAF-NEG -2,6) : le texte n'est pas reconstruit.

## 5. Seuils validés par le PO : valeurs absolues

Seuils : Qualité max ≥ 50 %, Équilibré ≥ 35 %, Rapide ≥ 20 % de (100 − VMAF Lanczos). Cibles calculées sur Lanczos **jeu A** (la définition du protocole), et en regard sur le **jeu B** (pour juger Broadcast, mesuré sur le jeu B).

| Clip | Lanczos A | Cibles A : Q / É / R | Lanczos B | Cibles B : Q / É / R | Broadcast | Atteint (base B) |
|---|---|---|---|---|---|---|
| BBB-FAST Kept | 92,67 | 96,33 / 95,23 / 94,13 | 90,59 | 95,30 / 93,88 / 92,47 | 94,70 | Équilibré et Rapide |
| BBB-PAN Kept | 83,39 | 91,70 / 89,20 / 86,71 | 81,50 | 90,75 / 87,98 / 85,20 | 84,12 | aucun (14 % < 20 %) |
| TOS-FACE | 92,81 | 96,41 / 95,33 / 94,25 | 88,53 (24 i/s) | 94,26 / 92,54 / 90,82 | 84,64 | aucun (sans SR) |
| TOS-DARK | 95,90 | 97,95 / 97,34 / 96,72 | 93,19 (24 i/s) | 96,60 / 95,57 / 94,55 | 91,10 | aucun (sans SR) |
| SYN-TEXT | 63,74 | 81,87 / 76,43 / 71,00 | 63,23 | 81,62 / 76,10 / 70,59 | 68,55 | aucun (14,5 %) |
| BBB-FAST Gen (info) | 69,23 | 84,62 / 80,00 / 75,39 | 68,63 | 84,31 / 79,61 / 74,90 | 89,52 | Qualité max (66,6 %) |
| BBB-PAN Gen (info) | 38,48 | 69,24 / 60,01 / 50,78 | 38,21 | 69,10 / 59,83 / 50,57 | 81,36 | Qualité max (69,8 %) |

(Les cibles Generated ne sont qu'indicatives : l'interpolation reste jugée par les règles propres du protocole §7.1, inchangées.)

Équivalents PSNR : +3,01 dB (Q), +1,87 dB (É), +0,97 dB (R) par rapport à Lanczos ; SSIM : (1 − SSIM) divisé par 2 (Q), × 0,65 (É), × 0,80 (R). Sur les images conservées, **Broadcast ne tient aucun de ces seuils PSNR/SSIM** (il est en dessous de Lanczos) : par ces mesures-là, les seuils validés sont plus exigeants que ce que fait Broadcast.

**Ce que cela implique pour la clause « Enhance ≥ Broadcast − 0,5 VMAF, SSIM et PSNR ≥ Broadcast »** (jeu B, clips valides) :
- VMAF : Enhance doit atteindre **94,2 sur BBB-FAST** (part de 43 % ; au niveau d'un préréglage entre Équilibré et Qualité max), **83,6 sur BBB-PAN**, **68,1 sur SYN-TEXT** (Broadcast − 0,5). BBB-PAN et SYN-TEXT sont donc **moins exigeants que le seuil Rapide** (20 %) : sur ces clips c'est le seuil en part de marge qui décide, pas Broadcast.
- SSIM et PSNR : Lanczos est déjà au-dessus de Broadcast sur les images conservées ; la clause est facile à tenir en fidélité et ne gêne pas.
- Risque : si Enhance reconstruit fidèlement (PSNR élevé), son VMAF ne dépassera peut-être pas celui d'un Broadcast accentué sur BBB-FAST. **Recommandation** : ajouter à la clause « VMAF-NEG d'Enhance ≥ VMAF-NEG de Broadcast − 0,5 » (c'est déjà inscrit au protocole §7.1) et décider avec le PO d'un test à l'aveugle ; ne pas se calquer sur le VMAF seul.

## 6. Verdict : Broadcast bat-il Lanczos ?

- **Interpolation (génération d'images) : oui, largement** (+21 et +43 VMAF sur les images générées).
- **Super-résolution : en VMAF seulement, de 2,6 à 5,3 points (14 à 44 % de la marge) sur les clips valides** ; **non en fidélité** (PSNR -0,1 à -11,5 dB, SSIM plus bas, VMAF-NEG plus bas sur les 3 clips). Résultat typique d'un filtre qui accentue.
- **TOS (2,39:1) : Broadcast ne fait pas de super-résolution du tout** (limite 4096 px) et passe en dessous de Lanczos (VMAF -2,1 à -3,9, PSNR -3,5 à -4,0 dB) : H.264 recompressé à 9-11 Mb/s en 1280×570, 48 i/s. À redemander avec les entrées bordées.
- Sorties lourdes : 23 à 65 Mb/s en H.264 8 bits.

## 7. Vitesse de Broadcast (estimation)

Note du PO : « moins d'une minute » pour les 5 vidéos (aucun chronométrage par clip). Images de sortie totales : 599 + 599 + 600 + 479 + 479 = 2 756. **Borne basse : ≥ 46 images de sortie par seconde** de bout en bout (chargement des modèles compris), en traitant la file en une fois ; ce n'est qu'une borne (le temps réel est inconnu), et SR 4K + génération n'ont été appliquées qu'à 3 clips sur 5. À comparer au budget d'Enhance, Qualité max ≥ 30 i/s et Équilibré ≥ 60 i/s : Broadcast est donc probablement au moins du niveau de Qualité max. Un chronométrage par clip (fichier par fichier, avec le compteur de Broadcast) donnerait une vraie valeur.

## 8. Actions

1. PO : repasser `tests/clips/in720-padded/TOS-FACE.mp4` et `TOS-DARK.mp4` dans Broadcast (mêmes réglages), sans renommer ; chronométrer chaque clip si possible. Option sans importance : 3840×2160 pour tous.
2. QA : brancher le recadrage central (3840×1710, y = 225) dans `Run-BroadcastReference.ps1` pour ces deux fichiers, puis remesurer.
3. Devops : committer les fichiers `tests/quality/*`, `docs/qa/protocole.md`, `docs/qa/resultats/` (JSON et rapports sans donnée personnelle ; les clips restent ignorés).
