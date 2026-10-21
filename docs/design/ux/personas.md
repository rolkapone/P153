# Enhance — Personas

Statut : v0.1 (ux-designer), 2026-10-21. Hypothèses signalées **[H]** quand elles ne viennent pas de `docs/vision.md`.

Point commun des trois : machine RTX 50 sous Windows 11, écran 4K ou en passe de l'être, fichiers privés qui ne doivent jamais quitter le PC, lancement « le soir, on revient demain ».

---

## 1. Carol, l'ingénieur passionné (persona principal, le PO)

| | |
|---|---|
| Profil | Ingénieur systèmes embarqués, 5 ans de C, apprend le C++. Utilise Claude Code. |
| Niveau technique | Élevé (codecs, débits, VMAF) mais veut **ne pas avoir à y penser** pour un usage courant. |
| Matériel | RTX 5070 12 Go, écran 4K, Windows 11. |
| Contexte | Traite des films 720p et des clips ; compare avec Broadcast 2.3 et son ancien pipeline Python. Lance une file avant de partir, regarde le résultat le lendemain. |

**Objectifs** : qualité maximale mesurable ; file de plusieurs fichiers sans surveillance ; HEVC 10 bits ou AV1 ; voir la vitesse réelle (images/s) ; comparer avant / après.
**Frustrations** : installation fragile (PyTorch cu128), absence de file d'attente chez Broadcast, sortie H.264 8 bits imposée, outils qui masquent ce qui se passe.
**Ce qu'il attend de l'UI** : démarrage en 2 gestes, chiffres honnêtes, options avancées présentes mais repliées, raccourcis clavier.

---

## 2. Hélène, l'archiviste familiale (persona de simplicité)

| | |
|---|---|
| Profil | 58 ans, numérise les DVD et cassettes de la famille ; a offert une RTX 5060 à son fils qui lui a monté un PC (**[H]**). |
| Niveau technique | Faible. Ne connaît ni HEVC, ni AV1, ni « débit ». Sait glisser un fichier et cliquer sur un bouton clair. |
| Matériel | RTX 50 d'entrée de gamme (8 à 12 Go **[H]**), téléviseur 4K au salon. |
| Contexte | Dépose 10 à 40 vieux fichiers d'un coup (AVI, MPG, MOV de caméscope). Laisse tourner la nuit. Veut les regarder sur la TV. |

**Objectifs** : « que ça ait l'air net sur la télé » ; ne rien perdre (son, sous-titres) ; confidentialité totale.
**Frustrations** : jargon, boîtes de dialogue qui demandent des choix qu'elle ne comprend pas, peur de « casser » l'original, ne pas retrouver le fichier produit.
**Ce qu'elle attend de l'UI** : un préréglage par défaut qui convient, un message rassurant (« votre fichier d'origine n'est pas modifié »), un bouton « Afficher dans le dossier », des erreurs qui disent quoi faire.

---

## 3. Maxime, le joueur rétro (persona de volume et de contenu spécifique)

| | |
|---|---|
| Profil | 31 ans, capture ses parties de jeux anciens (480p à 720p, 30 ou 60 i/s), les publie et les regarde en 4K (**[H]**). |
| Niveau technique | Moyen : connaît « 4K », « 60 fps », « H.265 », pas les réglages d'encodeur. |
| Matériel | RTX 5070 ou 5070 Ti, moniteur 4K 144 Hz. |
| Contexte | Captures longues (1 à 3 h) ; traite le week-end ; sensible à la netteté du pixel art, au texte à l'écran et aux artefacts d'interpolation. |

**Objectifs** : 4K propre sans halos ; fluidité (24/30 → 60) mais pas d'interpolation inutile sur ses captures déjà à 60 i/s ; durée estimée avant de lancer une capture de 2 h.
**Frustrations** : estimations de temps fausses, traitements de plusieurs heures qui échouent à 95 % sans explication, disque plein découvert trop tard.
**Ce qu'il attend de l'UI** : estimation de durée par fichier, progression fiable, erreurs précoces (espace disque avant de démarrer), reprise après incident (plus tard), aperçu rapide (plus tard).

---

## Questions ouvertes sur les personas
- Hélène suppose des cartes RTX 50 d'entrée de gamme (VRAM < 12 Go) : l'app doit-elle supporter 8 Go ? À confirmer par le budget VRAM (ENH-014).
