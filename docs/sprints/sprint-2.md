# Sprint 2 — planning (2026-10-29, validé par le PO le 2026-11-12, décisions n° 29 à 35)

**Objectif** : « Choisir le bon moteur d'agrandissement, convertir les couleurs sur le GPU et prouver que la super-résolution ne parle pas au réseau. »
Capacité : 15 pts (vitesse par défaut = Sprint 1). Budget : deux quotas hebdomadaires, alerte à 80 % (décision n° 31) ; devis de `socle:ressources` à recaler après la première story mesurée. Contenu : [proposition](sprint-2-proposition.md). Pas de story `[UI]` (décision n° 29).

## Stories
| Ordre | Story | Tags | Voie | Pts | Implémenteur | Dépendances |
|---|---|---|---|---|---|---|
| 1 | [ENH-041](../stories/ENH-041.md) NGX hors ligne + SEC-30 à SEC-36 | GPU | RENFORCÉE (chargement de DLL, réseau) | 5 (3 + 2 pour SEC-30 à 36) | gpu-engineer | ENH-040 (pont) |
| 2 | [ENH-042](../stories/ENH-042.md) Conversions couleur CUDA | GPU | STANDARD | 5 | gpu-engineer | ENH-008 |
| 3 | [ENH-053](../stories/ENH-053.md) Spike qualité SR (VSR q1-q4, Real-ESRGAN TRT) | ML + GPU | RENFORCÉE (modèle tiers, licence des poids) | 5 | ml-engineer (pas de code applicatif) | ENH-040 ; licence poids validée par secdevops avant usage |
| | **Total** | | | **15** | | |

**ENH-054 (flux PR, CI renforcée, méthodes ; 3 pts, hors sprint) précède le Sprint 2** (décision n° 35) ; les stories du sprint suivent le flux PR (branche poussée, PR « Closes #n », CI verte, tests GPU une fois en local ; décisions n° 33 et 34).

Ordre : ENH-041 d'abord (sécurité, SEC-35 doit précéder ENH-030, pont tout neuf). ENH-053 est indépendante des deux autres : elle peut démarrer en parallèle dès la licence validée. ENH-042 est indépendante aussi. **Maximum 2 agents lourds en parallèle** (ex. 041 + 053 : sécurité + ML). Précède le Sprint 3 : ENH-009 dépend de 053 et 042 ; ENH-010 de 042.

## Tâches (courtes)
- **ENH-041** : (1) init NGX sans `UpdateFeature`, app id 0, chemin de données, journal off, vérif `GetModuleFileNameW` ; (2) contrôle statique CI ; (3) `scripts/test-no-network-dynamic.ps1` (M1 pare-feu, M2 WFP, M3 capture) ; (4) SEC-30 à SEC-36 ([tracabilite-findings.md](../security/tracabilite-findings.md)) ; (5) rapport dans `docs/security/`.
- **ENH-042** : (1) référence CPU + test CI des coefficients ; (2) noyaux NV12/P010 <-> RGB ; (3) mire aller-retour, tailles limites (1x1, impaires, 8192x4320, SEC-13) ; (4) mesure 4K (<= 0,3 ms/image, VRAM <= 100 Mio).
- **ENH-053** : (1) secdevops : licence poids Real-ESRGAN dans `licences.md` ; (2) export ONNX local + moteur TensorRT hors ligne ; (3) VSR q1-q4 et Real-ESRGAN sur jeu B ; (4) QA : métriques vs Lanczos et Broadcast, vitesse (médiane de 5), VRAM ; (5) `docs/gpu/ENH-053.md` + ADR.

## Actions de la rétro du Sprint 1 appliquées
1. **Carte au repos** : toute mesure officielle (042, 053) précédée de `nvidia-smi` (charge < 5 %, VRAM de fond notée), applis graphiques fermées, baseline `main` rejouée dans la même session ; écart 5-15 % = « non concluant ». Devops ajoute le contrôle en tête de `Measure-Speed.ps1` (tâche préalable, hors points).
2. **Moins de cycles** : avant la revue, `clang-format` + `tidy.ps1` + SAST verts, auto-relecture sur la liste de motifs du reviewer (fournie en début de sprint) et un test « cas d'échec » par critère de mesure.
3. **Étapes courtes** : une étape de workflow par lancement d'agent ; état consigné dans le board ci-dessous après chaque étape ; <= 2 agents lourds en parallèle.
4. **Un worktree par story**, clips par jonction / `ENH_CLIPS_DIR` (jamais de copie), worktrees fusionnés supprimés (devops).
5. SEC-30 à SEC-36 absorbées par ENH-041 ; SEC-37 à 39 reportées à ENH-010.

## Board
| Story | Statut | Cycles de review | Notes |
|---|---|---|---|
| ENH-041 | TODO | 0 | RENFORCÉE : secdevops approfondi, threat model à mettre à jour |
| ENH-042 | TODO | 0 | |
| ENH-053 | TODO | 0 | Licence poids Real-ESRGAN à valider d'abord |

## Risques
- Pilote NVIDIA 621.14 installé, références mesurées avec 617.42 : la baseline `main` rejouée dans la session fait foi, pas les chiffres du Sprint 1.
- Quota au lancement (2026-11-12) : hebdomadaire 38 %, restreint 72 %, remise à zéro le 2026-11-13 à 15:00 UTC ; avant cette heure, seulement la licence Real-ESRGAN (secdevops) et la première étape d'ENH-041.
- Mesures GPU instables (5-15 %) : règle « carte au repos ». Si NGX télécharge une DLL : escalade immédiate au PO (ENH-041). Estimation d'ENH-041 (5 pts) à confirmer à la première étape.

## Revue / Rétro
À remplir en fin de sprint.
