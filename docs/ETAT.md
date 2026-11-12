# Enhance — ETAT (point d'entrée unique)

Mis à jour le 2026-11-12 (audit et validation du Sprint 2). Aucune story en cours : toutes les branches sont mergées. Dernier audit : [audits/2026-11-12.md](audits/2026-11-12.md).

## Sprint
- **Sprint 1 : terminé** (15 / 15 pts, 2026-10-25) : [sprints/sprint-1.md](sprints/sprint-1.md). Livré : ENH-004, ENH-008, ENH-040, ENH-047.
- **Sprint 2 : validé par le PO le 2026-11-12** (15 pts, décision n° 29), prêt à lancer. Objectif : « Choisir le bon moteur d'agrandissement, convertir les couleurs sur le GPU et prouver que la super-résolution ne parle pas au réseau. » Plan, voies, board : [sprints/sprint-2.md](sprints/sprint-2.md) (proposition : [sprint-2-proposition.md](sprints/sprint-2-proposition.md)).
- Budget de sprint = deux quotas hebdomadaires pour 15 pts, alerte à 80 % (décision n° 31). Relevé `/usage` du 2026-11-12 : hebdomadaire 38 %, restreint 72 %, remise à zéro le 2026-11-13 à 15:00 UTC. Devis produit par `socle:ressources` le 2026-10-29 (`equipe/ressources/devis-sprint-2.md`, non versionné), à recaler après la première story mesurée ; relevé `/usage` à refaire au lancement.
- Avant tout merge dans `main` : `format.ps1`, `tidy.ps1`, `sast.ps1` et `test.ps1` verts en local (décision n° 32).
- Plan de release, vitesse (15 pts / sprint), jalons J0-J5 : [backlog.md](backlog.md).

## Stories ouvertes (proposées au Sprint 2)
| Story | Titre | Tags | Pts | Voie | Statut | Prochain agent |
|---|---|---|---|---|---|---|
| [ENH-053](stories/ENH-053.md) | Spike qualité de la super-résolution | ML, GPU | 5 | RENFORCÉE | TODO (S2) | secdevops (licence des poids), puis ml-engineer |
| [ENH-042](stories/ENH-042.md) | Conversions couleur CUDA NV12/P010 <-> RGB | GPU | 5 | STANDARD | TODO (S2) | gpu-engineer |
| [ENH-041](stories/ENH-041.md) | NGX hors ligne (+ SEC-30 à SEC-36) | GPU | 5 | RENFORCÉE | TODO (S2, en 1er) | gpu-engineer |

Autres stories : [index dans backlog.md](backlog.md#index-des-stories) (54 fiches dans [stories/](stories/)). Non retenues pour le Sprint 2 : ENH-008b, ENH-009, ENH-010.

## Dernières décisions du PO (10 dernières ; toutes : [decisions.md](decisions.md))
23. **WER** (2026-10-24) : Enhance exclu des rapports d'erreurs Windows, aucun vidage mémoire ni envoi ; en cas de plantage, une ligne dans le journal local avec seulement un ...
24. **Seuils SSIM / PSNR validés** (2026-10-24) : PSNR +3,0 / +1,9 / +1,0 dB ; (1 - SSIM) x 0,5 / 0,65 / 0,8 vs Lanczos (Qualité max / Équilibré / Rapide) ; VMAF-NEG : part - 10 pts ; réajustable...
25. **Notification de fin de file** (2026-10-24) : au MVP, clignotement de l'icône dans la barre des tâches + progression sur l'icône + résumé dans l'app + son discret désactivable ; toast Windo...
26. **Budget de sprint** (2026-10-29) : le budget d'un sprint = 100 % du quota hebdomadaire de l'abonnement....
27. **Électricité** (2026-10-29) : coût de l'électricité compté pour 0 EUR....
28. **Relevé de départ** (2026-10-29) : relevé `/usage` de départ = 82 % du quota hebdomadaire consommé le 2026-10-29....
29. **Composition du Sprint 2** (2026-11-12) : ENH-041 + ENH-042 + ENH-053 = 15 pts, sans story `[UI]` ; J2 passe au Sprint 3.
30. **Poids Real-ESRGAN** (2026-11-12) : téléchargés par l'équipe après validation de la licence par secdevops, jamais commités.
31. **Budget de sprint** (2026-11-12) : 15 pts sur deux quotas hebdomadaires, alerte à 80 % (amende la n° 26).
32. **Contrôles locaux avant merge** (2026-11-12) : format, tidy, SAST et tests verts en local avant tout merge dans `main`.

## Seuils actifs (détail : [qa/seuils-reference.md](qa/seuils-reference.md), [qa/protocole.md](qa/protocole.md) §7)
- **Qualité** (écart VMAF 4K vs Lanczos, en part de la marge 100 - VMAF Lanczos) : Qualité max >= 50 %, Équilibré >= 35 %, Rapide >= 20 % ; SSIM/PSNR : PSNR +3,0 / +1,9 / +1,0 dB (décision n° 24). Pas de régression > 5 % vs `main`.
- **Vitesse** (i/s de sortie, médiane de 5 passes) : Qualité max >= 30, Équilibré >= 60, Rapide >= 100.
- **VRAM** : avertissement > 3 Gio, FAIL > 9 216 Mio ; dérive sur 10 fichiers <= 50 Mio.
- Référence à battre : NVIDIA Broadcast 2.3 (VMAF >= Broadcast - 0,5).

## Blocages et points ouverts
- Poids Real-ESRGAN : licence à valider par secdevops avant téléchargement (ENH-053, décision n° 30).
- Suivi des coûts : journal des agents sans id de story avant la v3 ; à vérifier sur la première story du Sprint 2.
- Mesures brutes de spike et de QA archivées dans `equipe/archives/2026-11-12-spike-qa-mesures.zip` (hors dépôt). `build/spike` (7 Go) et `build/qa-enh040` (1 Go) restent en place : leur suppression attend l'accord du PO. Après suppression, extraire l'archive pour rejouer les commandes `buildspike...` de [gpu/spike-phase2.md](gpu/spike-phase2.md).
- Pilote NVIDIA 621.14 installé, mesures de référence prises avec 617.42 : rejouer la baseline `main` avant toute comparaison de vitesse (Sprint 2).
- SEC-35 doit précéder ENH-030 ; suites LOW SEC-37 à SEC-40 : [ENH-047](stories/ENH-047-suites-secdevops.md).

## Où trouver quoi
- Décisions : [decisions.md](decisions.md) · Backlog (index + plan) : [backlog.md](backlog.md) · Vision : [vision.md](vision.md)
- Architecture : [architecture/index.md](architecture/index.md) (42 ADR rédigés) · Pipeline GPU : [gpu/architecture-pipeline.md](gpu/architecture-pipeline.md), [gpu/spike.md](gpu/spike.md)
- Sécurité : [security/threat-model.md](security/threat-model.md), [security/licences.md](security/licences.md), [security/zero-reseau.md](security/zero-reseau.md), [security/tracabilite-findings.md](security/tracabilite-findings.md)
- Design : [design/ui/design-system.md](design/ui/design-system.md), [design/ux/parcours.md](design/ux/parcours.md), [design/ux/criteres-transverses.md](design/ux/criteres-transverses.md)
- QA : [qa/protocole.md](qa/protocole.md), [qa/clips.md](qa/clips.md) · Revues : [reviews/](reviews/) · Sprints : [sprints/](sprints/) · Archives : [_archives/](_archives/)
