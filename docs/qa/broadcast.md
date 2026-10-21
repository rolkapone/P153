# Ce que le PO produit avec NVIDIA Broadcast 2.3 (et l'ancien pipeline Python)

Pourquoi : Broadcast n'a pas d'API, on ne peut pas le lancer par programme. Pour savoir si Enhance fait **au moins aussi bien**, on a besoin de ses résultats sur **exactement les mêmes petites vidéos**. C'est une tâche d'environ 30 minutes, à faire une fois les clips récupérés (`scripts/fetch-clips.ps1`).

Rien de ce que tu produis n'est commité ni partagé : tout reste dans `tests/clips/` (ignoré par git).

## 1. Ce qu'il faut passer dans Broadcast

Les fichiers d'entrée sont les petites vidéos **720p en MP4** (le « jeu B », créé par le script) dans `tests/clips/in720/`. **Ne donne pas** les vidéos 4K : l'idée est de partir de 720p.

| Priorité | Fichier d'entrée | Pourquoi ce clip |
|---|---|---|
| **Indispensable** | `BBB-FAST.mp4` | Dessin animé 3D, mouvements rapides (test de l'upscaling **et** de la génération d'images) |
| **Indispensable** | `TOS-FACE.mp4` | Visage, peau, film 24 i/s |
| Recommandé | `BBB-PAN.mp4` | Panoramique de caméra (génération d'images) |
| Recommandé | `TOS-DARK.mp4` | Scène sombre et bruit |
| Facultatif | `SYN-TEXT.mp4` | Texte fin défilant, 60 i/s (génération d'images à désactiver) |
| *Pas encore disponibles* (2e vague, non téléchargée) | `CROWD-50`, `SOL-2D`, `MERI-GRAIN` | Foule, animation 2D, grain |
| Facultatif, à l'œil seulement | `PO-RETRO` et `PO-ARCH` (tes extraits) | Pas de 4K d'origine pour mesurer ; sert à comparer visuellement |

Les 5 fichiers disponibles font 1280×720 (TOS : 1280×570, film 2,39:1, sans bandes noires), H.264 8 bits CRF 12, sans audio : `BBB-FAST` et `BBB-PAN` en 30 i/s, `TOS-FACE` et `TOS-DARK` en 24 i/s, `SYN-TEXT` en 60 i/s. Pour TOS, la sortie attendue de Broadcast est 3840×1710 : si Broadcast impose 3840×2160 avec bandes noires, note-le dans `notes.txt`.

Le minimum exigé par le backlog (ENH-029) est **2 clips** : les deux « Indispensable ».

## 2. Réglages à choisir dans Broadcast

L'écran exact peut différer un peu selon la version : **fais une capture d'écran des réglages** que tu utilises (sans nom d'utilisateur visible) et range-la dans `tests/clips/broadcast/reglages.png`. Principe : **la meilleure qualité possible, sans retouche d'image en plus**.

1. **Mode** : amélioration / upscaling d'un fichier vidéo (pas la webcam).
2. **Résolution de sortie** : **3840 × 2160 (4K)**.
3. **Qualité de l'upscaling** : le niveau **le plus élevé** proposé.
4. **Génération d'images (interpolation)** : **activée, 60 images/s** pour BBB-FAST, BBB-PAN et les clips à 24 i/s (TOS, SOL, MERI). **Désactivée** pour CROWD-50 (déjà 50 i/s) et SYN-TEXT (déjà 60 i/s) ; si Broadcast ne permet pas de la désactiver, note-le.
5. **Autres effets** (réduction de bruit, netteté, couleur, HDR, etc.) : **tous désactivés / à zéro**. On compare l'upscaling, pas un filtre d'embellissement.
6. **Format de sortie** : MP4 H.264 8 bits (le seul que Broadcast propose). Si un réglage de débit ou de qualité d'encodage existe, prends le plus élevé et **note la valeur**.

## 3. Où ranger les résultats et comment les nommer

Un fichier par clip, dans **`tests/clips/broadcast/`**, avec le **même nom que l'entrée** :

```
tests/clips/broadcast/BBB-FAST.mp4
tests/clips/broadcast/TOS-FACE.mp4
tests/clips/broadcast/BBB-PAN.mp4        (si fait)
tests/clips/broadcast/TOS-DARK.mp4       (si fait)
tests/clips/broadcast/reglages.png       (capture des réglages)
tests/clips/broadcast/notes.txt          (3 lignes : version exacte de Broadcast et du pilote, date,
                                          points inhabituels : réglage forcé, avertissement, durée de traitement par clip)
```

Ne renomme pas, ne recompresse pas, ne coupe pas les fichiers. Si Broadcast propose un nom automatique, renomme **uniquement** pour retrouver `<ID>.mp4`.

**Chronomètre** (facultatif mais utile) : note le temps que Broadcast met pour chaque clip, on le comparera à la vitesse d'Enhance.

## 4. Ancien pipeline Python (Real-ESRGAN + RIFE), si tu peux

Même principe : mêmes entrées (`tests/clips/in720/<ID>.mp4`), sortie 4K 60 i/s, rangée dans **`tests/clips/python/<ID>.mp4`**, avec les réglages notés (modèle Real-ESRGAN, facteur, RIFE). Au moins `BBB-FAST` et `TOS-FACE`. Facultatif : s'il t'est pénible à relancer (installation fragile cu128), on s'en passe.

## 5. Ensuite

Dis-moi simplement « les fichiers Broadcast sont prêts » : la QA lance la mesure (VMAF / SSIM / PSNR, mêmes images, même méthode que pour Enhance) et te présente le tableau. Tu feras aussi un **test à l'aveugle** : on te montrera, pour 3 ou 4 passages, deux images (A et B, tirées au hasard) sans te dire laquelle vient de Broadcast, et tu dis laquelle te plaît le plus.

