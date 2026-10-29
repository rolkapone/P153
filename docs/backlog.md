# Enhance — Backlog produit

Mise à jour du 2026-10-25 : **fin du Sprint 1** (15 / 15, plan de release recalé). Précédente : 2026-10-24, décision PO n° 20 (voir « Mouvements v0.5 »).
Version : v0.3 du 2026-10-21 (scrum-master, étape 7 du kickoff). Statut : v0.2 validée par le PO le 2026-10-21 ; **v0.3 à relire par le PO** (nouvelles stories, seuils QA, statuts du kickoff).
Les chiffres des critères `[GPU]` sont désormais **ancrés sur les mesures du spike** (`docs/gpu/spike.md`, phases 2 et 3) ; ceux qui restent des hypothèses sont marqués **[H]**.
Clips de référence : `BBB-FAST`, `BBB-PAN`, `TOS-FACE`, `TOS-DARK`, `SYN-TEXT` (`docs/qa/clips.md`) ; « REF-A » = `BBB-FAST` en jeu A tant que le PO n'a pas désigné un clip unique. « Sur la 5070 » = RTX 5070 12 Gio du PO, pilote 617.42.

## Plan de release

| Indicateur | Valeur |
|---|---|
| Total points MVP | **179** (126 en v0.2 ; +44 pts de stories nouvelles issues du spike, du threat model et de la revue des licences ; +1 net de ré-estimations, voir « Mouvements v0.3 » ; **+3 pts : ENH-008b**, redécoupage d'ENH-008 au planning du Sprint 1, voir « Mouvements v0.4 » ; **+5 pts : ENH-053**, spike qualité de la super-résolution, décision PO n° 20, voir « Mouvements v0.5 ») |
| Points livrés (kickoff = « sprint 0 », non compté dans la vitesse) | **38** : kickoff 23 (ENH-001 (5), ENH-002 (5), ENH-003 (4, DONE le 2026-10-24), ENH-005 (3), ENH-006 (3), ENH-007 (3)) + **Sprint 1 : 15 / 15** (ENH-008 (5, merge `0c2f748`), ENH-004 (5, merge `79a3e93`), ENH-040 (3, merge `986d7e3`), **ENH-047 (2, DONE le 2026-10-25, merge `76c559e`)**) |
| Points restants | **141** (179 - 38) |
| Vitesse | **15 pts / sprint, mesurée** (Sprint 1 : 15 / 15 ; un seul sprint terminé, donc un seul point de donnée ; le kickoff n'est pas un sprint) |
| Sprints restants estimés avant le MVP | **10** (141 / 15 = 9,4, arrondi au sprint supérieur : Sprints 2 à 11) ; fourchette **10 à 12** (stories `[UI]` à 3 cycles de revue, instabilité des mesures GPU, ré-estimations des spikes ENH-053 / ENH-045) |
| Dernière mise à jour | 2026-10-25 (**fin du Sprint 1** : 4 / 4 stories DONE, `main` = `76c559e` ; plan recalé sur la vitesse réelle ; SEC-37 à SEC-40 au backlog ; proposition de contenu du Sprint 2 ci-dessous, à valider par le PO) |

Répartition des points restants : l'ancienne répartition (143) n'était pas exactement sommable ; elle est **à recalculer au planning du Sprint 2** (Pipeline GPU : -2 pts avec ENH-047 livrée ; Fondations, sécurité et CI 8 : ENH-049, 050 ; le reste en file / préréglages / UI / transverse).
Total « Plus tard » : 52 pts + 8 pts (ENH-051) ; « Avant publication » : 3 pts (hors MVP).

### Jalons (indicatifs, à recaler à chaque fin de sprint)

| Jalon | Contenu | Sprint visé |
|---|---|---|
| J0 | Kickoff : spikes VSR / interpolation, protocole QA, threat model, licences, design system, CI locale (**fait**) | 0 |
| J1 | **Fenêtre Enhance** qui s'ouvre (Qt Quick, sombre, Mica), **décodage NVDEC** mesuré, **pont RTX Video** en production, `enhance-bench` v1 (agrandir une image en 4K) | 1 — **atteint le 2026-10-25** |
| J2 | **Premier fichier 4K de bout en bout** : décodage -> super-résolution -> NVENC HEVC 10 bits, i/s et VRAM mesurés par la QA | **3** (recalé : le spike ENH-053 et la sécurité NGX passent avant ENH-009 / ENH-010) |
| J3 | **Vrai 4K60** : RIFE (export maison, moteur TensorRT, coupures de plan) intégré, AV1, budget VRAM, audio / sous-titres | 5-6 |
| J4 | **L'application complète côté utilisateur** : file, glisser-déposer, préréglages, progression, pause / annulation, thèmes | 8-9 |
| J5 | **MVP** : robustesse (fuzzing), zéro réseau prouvé, FFmpeg minimal sans réseau, paquet portable, À propos / CLUF | **11** (141 pts / 15 par sprint) |

Ordre de bataille : Sprint 1 terminé (`docs/sprints/sprint-1.md`, 15 / 15). **Sprint 2 proposé** (voir « Proposition de contenu du Sprint 2 » ([sprints/sprint-2-proposition.md](sprints/sprint-2-proposition.md)), à valider par le PO) : ENH-053, ENH-042, ENH-041. Sprint 3 pressenti : ENH-009, ENH-010, ENH-008b (13 pts) + reliquat. ENH-008b (3 pts) : avant ENH-048, au plus tard Sprint 3. Sprints 4-5 : RIFE (ENH-043, ENH-044, ENH-012, ENH-046), AV1, file (ENH-016), journal (ENH-025).


## Où est passé le reste (ménage docs du 2026-10-29)

- Décisions du PO : [decisions.md](decisions.md)
- Fiches de stories : [stories/](stories/) ; suites secdevops : [ENH-040](stories/ENH-040-suites-secdevops.md), [ENH-047](stories/ENH-047-suites-secdevops.md)
- Proposition de contenu du Sprint 2 : [sprints/sprint-2-proposition.md](sprints/sprint-2-proposition.md)
- Seuils QA de référence : [qa/seuils-reference.md](qa/seuils-reference.md) ; critères UX transverses : [design/ux/criteres-transverses.md](design/ux/criteres-transverses.md)
- Traçabilité sécurité findings -> stories : [security/tracabilite-findings.md](security/tracabilite-findings.md)
- Questions pour le PO et points ouverts : [questions-po.md](questions-po.md)
- Historique : [mouvements v0.3-0.5](_archives/backlog-mouvements.md), [faits techniques du kickoff](_archives/backlog-faits-kickoff.md), [état des stories du kickoff](_archives/backlog-etat-kickoff.md)

## Index des stories

Voie : « — » = à attribuer par le scrum-master au planning (aucune voie n'était consignée avant le ménage). Sprint : 0 = kickoff.

### Epic A — Fondations techniques

| id | titre | tags | voie | pts | statut | sprint |
|---|---|---|---|---|---|---|
| [ENH-001](stories/ENH-001.md) | Spike RTX Video SDK : super-résolution 720p -> 4K | GPU | — | 5 | DONE (kickoff) | 0 |
| [ENH-002](stories/ENH-002.md) | Spike interpolation : RIFE vs NVIDIA Optical Flow FRUC | GPU | — | 5 | DONE (kickoff) | 0 |
| [ENH-003](stories/ENH-003.md) | Setup repo, build CMake/MSVC, CI et tests GPU locaux | — | — | 4 | DONE (2026-10-24) | — |
| [ENH-004](stories/ENH-004.md) | Squelette de l'application Qt Quick + prototype Mica | UI | — | 5 | DONE (2026-10-23) | 1 |
| [ENH-005](stories/ENH-005.md) | Protocole qualité et clips de référence | — | — | 3 | DONE (kickoff) | 0 |
| [ENH-006](stories/ENH-006.md) | Threat model et registre des licences | — | — | 3 | DONE (kickoff) | 0 |
| [ENH-007](stories/ENH-007.md) | Design system et tokens | UI | — | 3 | DONE (kickoff) | 0 |
| [ENH-040](stories/ENH-040.md) | DLL pont RTX Video et chargement différé des DLL NVIDIA | GPU | — | 3 | DONE (2026-10-24) | 1 |
| [ENH-041](stories/ENH-041.md) | NGX hors ligne : neutraliser la mise à jour et le prouver | GPU | — | 3 | À faire | — |
| [ENH-042](stories/ENH-042.md) | Conversions couleur CUDA (NV12 / P010 <-> RGB) | GPU | — | 5 | À faire | — |
| [ENH-043](stories/ENH-043.md) | Export ONNX maison de RIFE depuis les poids MIT | GPU | — | 5 | À faire | — |
| [ENH-044](stories/ENH-044.md) | Moteur TensorRT : construction au premier traitement, cache, exécution | GPU | — | 5 | À faire | — |
| [ENH-045](stories/ENH-045.md) | Comparaison des modèles d'interpolation : RIFE 4.25, FILM, IFRNet, EMA-VFI | GPU | — | 5 | À faire | — |
| [ENH-046](stories/ENH-046.md) | Détection de coupure de plan | GPU | — | 3 | À faire | — |
| [ENH-047](stories/ENH-047.md) | `enhance-bench` : banc en ligne de commande pour la QA | GPU | — | 2 | DONE (2026-10-25) | 1 |
| [ENH-048](stories/ENH-048.md) | FFmpeg minimal sans réseau (port vcpkg) | GPU | — | 5 | À faire | — |
| [ENH-049](stories/ENH-049.md) | Garde-fous CI et chaîne de livraison (reliquat devops du kickoff) | — | — | 5 | À faire | — |
| [ENH-050](stories/ENH-050.md) | À propos, mentions légales et CLUF | UI | — | 3 | À faire | — |

### Epic B — Pipeline vidéo GPU

| id | titre | tags | voie | pts | statut | sprint |
|---|---|---|---|---|---|---|
| [ENH-008](stories/ENH-008.md) | Démuxage FFmpeg et décodage NVDEC | GPU | — | 5 | DONE (2026-10-23) | 1 |
| [ENH-008b](stories/ENH-008b.md) | Matrice de formats étendue | GPU | — | 3 | À faire | — |
| [ENH-053](stories/ENH-053.md) | Spike qualité de la super-résolution | GPU | — | 5 | À faire | — |
| [ENH-009](stories/ENH-009.md) | Super-résolution RTX Video SDK dans le pipeline | GPU | — | 5 | À faire | — |
| [ENH-010](stories/ENH-010.md) | Encodage NVENC HEVC 8 et 10 bits | GPU | — | 5 | À faire | — |
| [ENH-011](stories/ENH-011.md) | Encodage NVENC AV1 | GPU | — | 3 | À faire | — |
| [ENH-012](stories/ENH-012.md) | Interpolation d'images vers 60 images/s | GPU | — | 5 | À faire | — |
| [ENH-013](stories/ENH-013.md) | Conservation de l'audio, des sous-titres et des métadonnées | GPU | — | 3 | À faire | — |
| [ENH-014](stories/ENH-014.md) | Budget VRAM et absence de fuite | GPU | — | 3 | À faire | — |
| [ENH-015](stories/ENH-015.md) | Robustesse face aux fichiers malformés | GPU | — | 5 | À faire | — |

### Epic C — File d'attente et préréglages

| id | titre | tags | voie | pts | statut | sprint |
|---|---|---|---|---|---|---|
| [ENH-016](stories/ENH-016.md) | Modèle de la file d'attente | — | — | 5 | À faire | — |
| [ENH-017](stories/ENH-017.md) | Glisser-déposer et ajout à la file | UI | — | 5 | À faire | — |
| [ENH-018](stories/ENH-018.md) | Préréglages et choix du codec | UI | — | 5 | À faire | — |
| [ENH-019](stories/ENH-019.md) | Exécution séquentielle de la file | GPU | — | 5 | À faire | — |
| [ENH-020](stories/ENH-020.md) | Progression : images/s et temps restant | UI | — | 5 | À faire | — |
| [ENH-021](stories/ENH-021.md) | Pause et annulation | UI GPU | — | 5 | À faire | — |
| [ENH-022](stories/ENH-022.md) | Réordonnancement et persistance de la file | UI | — | 5 | À faire | — |
| [ENH-023](stories/ENH-023.md) | Ouvrir le fichier produit | UI | — | 2 | À faire | — |
| [ENH-024](stories/ENH-024.md) | Dossier de sortie, nommage et réglages | UI | — | 3 | À faire | — |
| [ENH-025](stories/ENH-025.md) | Journalisation locale | — | — | 2 | À faire | — |
| [ENH-026](stories/ENH-026.md) | Thèmes sombre et clair, accessibilité | UI | — | 5 | À faire | — |
| [ENH-027](stories/ENH-027.md) | Interface en français | UI | — | 2 | À faire | — |
| [ENH-028](stories/ENH-028.md) | Garde-fou « zéro réseau » | — | — | 3 | À faire | — |
| [ENH-029](stories/ENH-029.md) | Banc d'essai qualité automatisé vs Lanczos et Broadcast | — | — | 5 | À faire | — |
| [ENH-030](stories/ENH-030.md) | Version portable pour la démo | — | — | 5 | À faire | — |
| [ENH-031](stories/ENH-031.md) | Détection des prérequis (carte, pilote) | UI | — | 3 | À faire | — |

### Plus tard

Contraintes d'architecture à tenir dès le MVP : étapes du pipeline en modules chaînés (ajout d'une étape HDR sans réécriture), description d'un job sérialisable (reprise, dossiers), préréglages en données, conteneur de sortie paramétrable, décodeur capable de fournir des images 10 bits, RIFE capable de travailler en RGB flottant (entrée P010 sans perte).

| id | titre | tags | voie | pts | statut | sprint |
|---|---|---|---|---|---|---|
| [ENH-032](stories/ENH-032.md) | SDR -> HDR10 (RTX Video SDK) | GPU UI | — | 8 | À faire | — |
| [ENH-033](stories/ENH-033.md) | Comparaison avant / après côte à côte | UI GPU | — | 8 | À faire | — |
| [ENH-034](stories/ENH-034.md) | Préréglages personnalisés | UI | — | 5 | À faire | — |
| [ENH-035](stories/ENH-035.md) | Options avancées : débit, résolution, cadence cibles, MP4/MKV | UI GPU | — | 8 | À faire | — |
| [ENH-036](stories/ENH-036.md) | Aperçu rapide sur quelques secondes | UI GPU | — | 5 | À faire | — |
| [ENH-037](stories/ENH-037.md) | Traitement de dossiers entiers | UI | — | 5 | À faire | — |
| [ENH-038](stories/ENH-038.md) | Reprise après plantage | GPU | — | 8 | À faire | — |
| [ENH-039](stories/ENH-039.md) | Installeur Windows propre | — | — | 5 | À faire | — |
| [ENH-051](stories/ENH-051.md) | Sondage des fichiers dans un processus restreint | GPU | — | 8 | À faire | — |

### Avant publication

| id | titre | tags | voie | pts | statut | sprint |
|---|---|---|---|---|---|---|
| [ENH-052](stories/ENH-052.md) | Check-list avant diffusion publique | — | — | 3 | À faire | — |
