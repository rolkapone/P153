# Enhance — Vision produit

Statut : v0.2 du 2026-10-21 (scrum-master), **validée par le PO le 2026-10-21**.
Convention : toute valeur marquée **[H]** est une **hypothèse** à confirmer par les spikes GPU (ENH-001, ENH-002) et le protocole qualité (ENH-005).

## Décisions du PO (2026-10-21)

1. **Cadence** : jamais d'interpolation au-delà de 60 i/s ; une source >= 50 i/s n'est pas interpolée ; 24 i/s -> vrai 60 i/s interpolé. Toute source strictement sous 50 i/s (y compris 31-49 i/s) est interpolée vers 60 i/s.
2. **Conteneur MVP** : MP4 ; bascule automatique en MKV (avec message) si une piste de sous-titres est incompatible MP4.
3. **Clips de référence** : clips libres (Blender, Xiph) + 1 ou 2 extraits des archives du PO, jamais commités.
4. **Publication** : gratuite, code privé pour l'instant ; FFmpeg LGPL en liaison dynamique.
5. **Vitesse** : si la 5070 ne tient pas 60 i/s, la qualité passe d'abord, la vitesse est mesurée et affichée ; l'interpolation n'est dégradée que dans « Rapide ».
6. **Quitter pendant un traitement** : seule exception à « pas de confirmation » ; la croix réduit l'app dans la barre des tâches sans arrêter le traitement, Ctrl+Q ouvre la feuille « Quitter et abandonner le traitement en cours ? ».
7. **Toolchain** : Visual Studio 2026 (MSVC v145) au lieu de VS 2022 ; CUDA Toolkit 13.x (<= 13.4, pilote 617.42) devant accepter ce compilateur hôte.

## 1. Problème

Les vidéos anciennes ou en basse définition (archives familiales, captures de jeux rétro, films 720p) paraissent floues sur un écran 4K. Les solutions actuelles :
- **NVIDIA Broadcast 2.3** (sorti le 14/10/2026) : upscaling de fichiers et génération d'images, mais sortie MP4 H.264 8 bits uniquement, pas de file d'attente, aucun choix d'encodage.
- **Chaînes Python** (FFmpeg + Real-ESRGAN + RIFE + Gradio, dont l'ancien pipeline du PO) : bonne qualité mais installation fragile (PyTorch cu128), lentes, sans confort d'usage.
- **Outils en ligne** : envoient les vidéos dans le cloud, incompatible avec des archives privées.

## 2. Utilisateurs cibles

1. **Carol (PO)** : premier utilisateur, machine de référence RTX 5070 (12 Go).
2. **Passionnés de vidéo** avec une GeForce RTX 50 : veulent une qualité maximale sans ligne de commande.
3. **Archivistes familiaux** : DVD numérisés, caméscopes, vieux téléphones ; exigent la confidentialité (rien ne sort du PC).
4. **Joueurs rétro** : captures 720p ou moins à remonter en 4K fluide.

## 3. Proposition de valeur

Enhance est une application Windows, **en français**, **100 % locale** (aucun appel réseau, aucune télémétrie, aucun compte), qui transforme des fichiers vidéo en 4K 60 images/s avec la super-résolution native du RTX Video SDK, une interpolation d'images et un encodage matériel NVENC.

| | NVIDIA Broadcast 2.3 | Enhance (MVP) |
|---|---|---|
| Formats d'entrée | limités | tous les formats lisibles par FFmpeg |
| Sortie | MP4 H.264 8 bits | HEVC ou AV1, 8 ou 10 bits |
| File d'attente | non | oui (réordonnancement, pause, annulation) |
| Préréglages | non | Qualité max / Équilibré / Rapide |
| Audio et sous-titres | -- | conservés |
| Qualité mesurée | non publiée | VMAF / SSIM / PSNR sur clips de référence, au moins égaux à Broadcast |
| Confidentialité | locale | locale, vérifiée par test « zéro réseau » |
| Design | -- | qualité « Apple », sombre et clair |

## 4. Périmètre

### MVP
- Glisser-déposer d'un ou plusieurs fichiers vidéo (tout format lu par FFmpeg), ajout à une file.
- Préréglages « Qualité max », « Équilibré », « Rapide », avec choix du codec HEVC ou AV1 (10 bits possible).
- Traitement séquentiel : décodage NVDEC, super-résolution RTX Video SDK, interpolation d'images vers 60 i/s (jamais au-delà ; source >= 50 i/s non interpolée), encodage NVENC.
- Conservation de l'audio et des sous-titres ; sortie MP4, bascule automatique en MKV (avec message) si un sous-titre est incompatible MP4.
- Progression (images/s, temps restant), pause, annulation, réordonnancement, ouverture du fichier produit.
- Interface française, thèmes sombre et clair, animations fluides, accessibilité de base.
- Journalisation locale, gestion d'erreurs sur fichiers malformés, version portable pour la démo.

### Vision long terme (à prévoir dans l'architecture, voir « Plus tard » du backlog)
SDR -> HDR10 (RTX Video SDK), comparaison avant/après côte à côte, préréglages personnalisés et options avancées (débit, résolution et cadence cibles, conteneur MP4/MKV), aperçu rapide sur quelques secondes, traitement de dossiers entiers, reprise après plantage, installeur Windows propre.

### Hors périmètre explicite (MVP et au-delà tant que le PO ne dit pas le contraire)
Traitement temps réel / lecteur vidéo, montage vidéo, GPU non NVIDIA ou antérieurs à RTX 50, macOS/Linux, fonctions réseau (mise à jour automatique, cloud, télémétrie), téléchargement de modèles à l'exécution, édition audio.

## 5. Objectifs chiffrés (tous **[H]** jusqu'au spike)

Méthode de référence : prendre un clip 4K source, le réduire en 720p (Lanczos), le traiter avec Enhance, comparer au 4K d'origine (VMAF modèle 4K, SSIM, PSNR sur la luminance). Détail dans `docs/qa/protocole.md` (ENH-005).

| Axe | Objectif | Statut |
|---|---|---|
| Qualité vs Lanczos | VMAF >= Lanczos + 5 pts, SSIM >= Lanczos + 0,02, PSNR >= Lanczos + 1,5 dB (préréglage Qualité max) | **[H]** |
| Qualité vs Broadcast 2.3 | VMAF, SSIM, PSNR >= Broadcast sur les mêmes clips (écart toléré -0,5 VMAF, car sortie Broadcast en 8 bits) | **[H]** |
| Vitesse, Équilibré | >= 60 images/s de sortie en 720p -> 4K60 (traitement au moins en temps réel) | **[H]** |
| Vitesse, Rapide | >= 100 images/s de sortie | **[H]** |
| Vitesse, Qualité max | >= 30 images/s de sortie (acceptable : plus lent que le temps réel) | **[H]** |
| Arbitrage vitesse / qualité | Si 60 i/s n'est pas tenu : qualité d'abord, vitesse mesurée et affichée ; interpolation dégradée seulement en « Rapide » | décision PO |
| VRAM par job | <= 9 Gio (9 216 Mio) pic mesuré, soit 3 Gio de marge sur 12 Go pour Windows et l'affichage de l'app | **[H]** |
| Fuite VRAM | 0 Mio de dérive après 10 fichiers à la suite | exigence DoD |
| Réseau | 0 appel réseau, vérifié par test automatisé et scan | exigence ferme |
| Interface | 60 images/s d'animation, y compris pendant un traitement GPU | exigence ferme |
| Régression | pas de perte de vitesse ou de qualité > 5 % par rapport à `main` | exigence DoD |

## 6. Contraintes

- 100 % local, aucun compte, aucune télémétrie.
- Backend C++20 sans exception ; front Qt 6 / Qt Quick par défaut (à confirmer par ADR).
- Windows 11, GeForce RTX 50 (Blackwell, `sm_120`), CUDA Toolkit >= 12.8, pilote récent exigé par le RTX Video SDK.
- 12 Go de VRAM sur la machine de référence.
- Le SDK NVIDIA et les modèles ne sont ni téléchargés ni commités sans validation de licence par secdevops ; les téléchargements avec compte NVIDIA sont faits par le PO.
- La CI GitHub n'a pas de GPU : tests GPU en local.
- Publication gratuite, code privé pour l'instant ; FFmpeg LGPL en liaison dynamique (pas de GPL ni nonfree).
- Clips de référence : libres (Blender, Xiph) + 1 ou 2 extraits des archives du PO, jamais commités.
- Identité git et fichiers exclus du dépôt : voir `CLAUDE.md`.

## 7. Risques principaux

| # | Risque | Impact | Parade |
|---|---|---|---|
| R1 | Le RTX Video SDK ne fournit pas la génération d'images | Cadence 60 i/s impossible sans autre brique | Spike ENH-002 : RIFE (TensorRT / CUDA) vs NVIDIA Optical Flow FRUC ; ADR |
| R2 | Licence de redistribution du SDK, des modèles RIFE, de FFmpeg (LGPL/GPL, codecs) | Publication impossible | Registre des licences dès le début (ENH-006), FFmpeg en liaison dynamique LGPL |
| R3 | Vitesse insuffisante (super-résolution + interpolation + encodage sur une 5070) | Objectif 60 i/s non atteint | Budgets par étape, pipeline asynchrone, préréglages qui sacrifient l'interpolation ; objectif révisable avec le PO |
| R4 | VRAM de 12 Go saturée en 4K (images décodées, tampons, modèle, encodeur) | Plantages | Budget fixé et mesuré (ENH-014), file de tampons bornée |
| R5 | Robustesse face aux fichiers malformés | Plantage, faille de sécurité | Fuzzing léger et gestion d'erreur par `std::expected` (ENH-015) |
| R6 | Artefacts de l'interpolation (scènes coupées, texte) | Qualité perçue | Détection de changement de plan, mesures VMAF, seuils QA |
| R7 | Qualité « Apple » difficile à tenir sur Qt Quick sous Windows | Écran non DONE | Design system et tokens tôt (ENH-007), contrôle ui-designer à chaque story UI |
| R8 | Sous-titres images (PGS) ou formats exotiques non copiables dans MP4 | Perte d'information | MP4 par défaut, bascule automatique en MKV avec message clair (décision PO n°2) |
| R9 | Montée en compétence C++ du PO | Rythme | Rubrique « À retenir en C++ » à chaque story, code idiomatique et petit |
| R10 | Disponibilité du SDK RTX Video (compte NVIDIA) | Blocage du spike | Le PO télécharge ; l'équipe indique quoi récupérer et où le poser |

## 8. Indicateurs de succès du MVP

1. Un clip 720p 30 i/s sort en 4K 60 i/s HEVC 10 bits sur la RTX 5070 avec les seuils du §5 atteints.
2. Une file de 10 fichiers se traite sans intervention, sans fuite VRAM.
3. Le PO utilise l'app seul, de bout en bout, sans documentation.
4. Zéro octet émis sur le réseau (vérifié).
