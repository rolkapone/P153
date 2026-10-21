# Enhance — Outils de mesure QA (hors dépôt)

Dernière mise à jour : 2026-10-21 (qa-tester). Accord du PO : 2026-10-21. Licence validée par secdevops (outil de mesure GPL, jamais lié, jamais livré, jamais utilisé par l'application).

## FFmpeg de mesure

| Élément | Valeur |
|---|---|
| Build | BtbN/FFmpeg-Builds, release `autobuild-2026-10-21-13-06` (épinglée par la date) |
| Fichier | `ffmpeg-n8.1.3-20-gc8de997cb1-win64-gpl-8.1.zip` (statique, GPL) |
| SHA-256 de l'archive | `3c018ceab05450a5ca7b628d102eee676ef6124580fab0c0bc033570d70523d5` (identique à `checksums.sha256` de la release) |
| Version | `ffmpeg n8.1.3-20-gc8de997cb1-20261021` (gcc 16.2.0) |
| libvmaf | compilée dedans (`--enable-libvmaf`), révision `86da14d` (lue dans le JSON de libvmaf) |
| libx264 | `--enable-libx264` (jeu B) |
| Modèles VMAF | intégrés au binaire : `vmaf_4k_v0.6.1` et `vmaf_4k_v0.6.1neg` (vérifiés utilisables, `version=` en ligne de commande, aucun fichier externe) |
| Emplacement | `C:\Tools\ffmpeg\bin\` (ffmpeg, ffprobe, ffplay) + `LICENSE.txt` ; passé aux scripts par `-FfmpegDir C:\Tools\ffmpeg` |

Remarque de syntaxe FFmpeg 8 : `-filter_complex_script` est obsolète, les scripts utilisent `-/filter_complex <fichier>`.
Comparer deux rapports n'a de sens que si ces trois valeurs (ffmpeg, libvmaf, modèle) sont identiques.
