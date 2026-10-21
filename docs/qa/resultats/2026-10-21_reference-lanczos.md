# Mesure de reference Lanczos (plancher) - 2026-10-21

Machine : NVIDIA GeForce RTX 5070, pilote 617.42, Windows 10.0.26200.0. Depot : 7f54e6c.
Outils : ffmpeg version n8.1.3-20-gc8de997cb1-20261021 Copyright (c) 2000-2026 the FFmpeg developers ; libvmaf 86da14d ; modeles vmaf_4k_v0.6.1 et vmaf_4k_v0.6.1neg. Details : docs/qa/outils.md.

| Clip | Reference | Jeu | Images comparees | VMAF | VMAF-NEG | VMAF p1 | VMAF min | SSIM Y | PSNR Y (dB) | Alignement |
|---|---|---|---|---|---|---|---|---|---|---|
| BBB-FAST | Lanczos Generated | A | 300 | 69.23 | 67.09 | 12.97 | 5.55 | 0.9530 | 31.70 | ok |
| BBB-FAST | Lanczos Kept | A | 300 | 92.67 | 91.33 | 80.83 | 80.83 | 0.9997 | 44.45 | ok |
| BBB-FAST | Lanczos Generated | B | 300 | 68.63 | 66.50 | 12.95 | 5.56 | 0.9529 | 31.51 | ok |
| BBB-FAST | Lanczos Kept | B | 300 | 90.59 | 89.05 | 80.48 | 80.47 | 0.9993 | 42.77 | ok |
| BBB-FAST | Minterpolate Generated | A | 300 | 81.60 | - | 19.91 | 5.55 | 0.9851 | 37.66 | ok |
| BBB-FAST | Minterpolate Kept | A | 300 | 92.56 | - | 80.83 | 80.83 | 0.9997 | 44.38 | ok |
| BBB-PAN | Lanczos Generated | A | 300 | 38.48 | 37.42 | 14.07 | 13.58 | 0.9591 | 27.35 | ok |
| BBB-PAN | Lanczos Kept | A | 300 | 83.39 | 81.98 | 81.43 | 81.27 | 0.9992 | 33.28 | ok |
| BBB-PAN | Lanczos Generated | B | 300 | 38.21 | 37.18 | 14.08 | 13.53 | 0.9587 | 27.37 | ok |
| BBB-PAN | Lanczos Kept | B | 300 | 81.50 | 80.18 | 78.76 | 78.20 | 0.9988 | 33.11 | ok |
| BBB-PAN | Minterpolate Generated | A | 300 | 65.76 | - | 46.53 | 36.00 | 0.9958 | 31.59 | ok |
| BBB-PAN | Minterpolate Kept | A | 300 | 82.97 | - | 81.22 | 36.26 | 0.9989 | 33.24 | ok |
| SYN-TEXT | Lanczos All | A | 600 | 63.74 | 62.34 | 62.23 | 62.06 | 0.9943 | 24.39 | ok |
| SYN-TEXT | Lanczos All | B | 600 | 63.23 | 61.62 | 61.32 | 60.74 | 0.9941 | 24.38 | ok |
| TOS-DARK | Lanczos Aligned24 | A | 120 | 95.90 | 94.63 | 90.36 | 89.62 | 0.9996 | 43.58 | ok |
| TOS-DARK | Lanczos Aligned24 | B | 120 | 94.52 | 93.16 | 87.57 | 86.61 | 0.9985 | 41.91 | ok |
| TOS-DARK | Lanczos All | B | 240 | 93.19 | 91.84 | 85.63 | 85.42 | 0.9982 | 41.72 | ok |
| TOS-FACE | Lanczos Aligned24 | A | 120 | 92.81 | 91.41 | 89.74 | 89.57 | 0.9995 | 39.38 | ok |
| TOS-FACE | Lanczos Aligned24 | B | 120 | 89.32 | 87.71 | 86.09 | 86.08 | 0.9977 | 38.72 | ok |
| TOS-FACE | Lanczos All | B | 240 | 88.53 | 86.91 | 84.57 | 84.13 | 0.9976 | 38.64 | ok |


## Lecture (qa-tester, mise à jour 2026-10-21 soir)

- **Correction d'un bug de mesure** : `Measure-Quality.ps1` appariait mal les images d'un mp4 et d'un mkv (arrondi des horodatages en ms, une image sur trois décalée). Corrigé (`settb=1/30,setpts=N`), **toutes les mesures ont été refaites**. Effets : SSIM Kept BBB-FAST jeu A 0,9984 → 0,9997 ; minimum VMAF Kept BBB-FAST 60 → 80,8 et BBB-PAN 2,5 → 81,3 (les « images isolées très mauvaises » du premier rapport étaient des artefacts) ; VMAF Kept +0,4 à +0,8. Les valeurs TOS (Aligned24) et SYN-TEXT sont inchangées à 0,01 près. Les deux passes sur les mêmes entrées donnent le même résultat à la décimale (VMAF déterministe).
- **Entrées** : vérité terrain = extraits de 10 s des sources 4K (FFV1, BT.709 limité) ; entrée 720p = Lanczos 4K→720p (jeu A FFV1 ; jeu B H.264 CRF 12). Lanczos 720p→4K, cadence 60 i/s par **duplication** ; `Minterpolate` = référence basse indicative.
- **Kept** = images 4K présentes en entrée ; **Generated** = images retirées ; **Aligned24** = 1 image sur 5 à 60 i/s (120 images) ; **All** (TOS, jeu B) = les 240 images à 24 i/s, comparables à Broadcast ; **All** (SYN-TEXT) = 600 images.
- **Seuils §7.1** : marge VMAF restante de Lanczos (jeu A) : BBB-FAST 7,3 ; BBB-PAN 16,6 ; TOS-FACE 7,2 ; TOS-DARK 4,1 ; SYN-TEXT 36,3. D'où les seuils en part de la marge validés par le PO (voir protocole §7.1 pour les VMAF cibles).
- **Interpolation** : duplication Generated 69,2 (BBB-FAST) et 38,5 (BBB-PAN) ; `minterpolate` 81,6 et 65,8, mais 1er centile 19,9 (FAST) et minimum 5,6 : les coupures de plan (3 dans BBB-FAST) sont mauvaises pour toute méthode.
- **Jeu B** (H.264 CRF 12) coûte 0,5 à 3,5 VMAF par rapport au jeu A : les comparaisons avec Broadcast se font **uniquement entre sorties du jeu B**.
