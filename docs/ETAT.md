# Enhance — ETAT (point d'entrée unique)

Mis à jour le 2026-10-29 (ménage docs). `main` = f3edcbc avant le ménage. Aucune story en cours : toutes les branches sont mergées.

## Sprint
- **Sprint 1 : terminé** (15 / 15 pts, 2026-10-25) : [sprints/sprint-1.md](sprints/sprint-1.md). Livré : ENH-004, ENH-008, ENH-040, ENH-047.
- **Sprint 2 : non démarré**, en attente de validation PO. Objectif proposé : « Choisir le bon moteur d'agrandissement, convertir les couleurs sur le GPU et prouver que la super-résolution ne parle pas au réseau. » Contenu proposé : [sprints/sprint-2-proposition.md](sprints/sprint-2-proposition.md).
- Budget de sprint = 100 % du quota hebdomadaire (décision n° 26) ; relevé de départ 82 % au 2026-10-29 (n° 28). Devis CFO à produire avant lancement.
- Plan de release, vitesse (15 pts / sprint), jalons J0-J5 : [backlog.md](backlog.md).

## Stories ouvertes (proposées au Sprint 2)
| Story | Titre | Tags | Pts | Voie | Statut | Prochain agent |
|---|---|---|---|---|---|---|
| [ENH-053](stories/ENH-053.md) | Spike qualité de la super-résolution | GPU | 5 | à attribuer | à planifier | scrum-master (planning) |
| [ENH-042](stories/ENH-042.md) | Conversions couleur CUDA NV12/P010 <-> RGB | GPU | 5 | à attribuer | à planifier | scrum-master |
| [ENH-041](stories/ENH-041.md) | NGX hors ligne (+ SEC-30 à SEC-36) | GPU | 3 (+ suites) | RENFORCÉE probable | à planifier | scrum-master |

Autres stories : [index dans backlog.md](backlog.md#index-des-stories) (54 fiches dans [stories/](stories/)). Non retenues pour le Sprint 2 : ENH-008b, ENH-009, ENH-010.

## Dernières décisions du PO (10 dernières ; toutes : [decisions.md](decisions.md))
19. **Aucune vraie frappe clavier / clic sur le poste du PO** (2026-10-23) : aucun agent n'envoie de vraies frappes clavier ni de clics sur le poste du PO (incident QA d'ENH-004) ; les tests clavier sont automatisés ou faits ...
20. **Qualité de la super-résolution et seuil de débit** (2026-10-24) : ENH-040 validée sur le périmètre du pont, critère de qualité VSR déplacé dans ENH-009 ; **ENH-053 créée** : spike « niveaux VSR 1 à 4 sur ...
21. **Traçabilité des commits et équipe** (2026-10-24) : chaque commit porte une ligne `Agent: <rôle>` (rôle de l'agent auteur du changement) ; le README comporte une section « Équipe ». Cette ligne n'...
22. **Poids RIFE** (2026-10-24) : l'équipe télécharge elle-même `flownet.pkl` (RIFE 4.25, Practical-RIFE, MIT) au sprint 3, SHA-256 consigné, README + `LICENSE` archivés, jamais...
23. **WER** (2026-10-24) : Enhance exclu des rapports d'erreurs Windows, aucun vidage mémoire ni envoi ; en cas de plantage, une ligne dans le journal local avec seulement un ...
24. **Seuils SSIM / PSNR validés** (2026-10-24) : PSNR +3,0 / +1,9 / +1,0 dB ; (1 - SSIM) x 0,5 / 0,65 / 0,8 vs Lanczos (Qualité max / Équilibré / Rapide) ; VMAF-NEG : part - 10 pts ; réajustable...
25. **Notification de fin de file** (2026-10-24) : au MVP, clignotement de l'icône dans la barre des tâches + progression sur l'icône + résumé dans l'app + son discret désactivable ; toast Windo...
26. **Budget de sprint** (2026-10-29) : le budget d'un sprint = 100 % du quota hebdomadaire de l'abonnement....
27. **Électricité** (2026-10-29) : coût de l'électricité compté pour 0 EUR....
28. **Relevé de départ** (2026-10-29) : relevé `/usage` de départ = 82 % du quota hebdomadaire consommé le 2026-10-29....

## Seuils actifs (détail : [qa/seuils-reference.md](qa/seuils-reference.md), [qa/protocole.md](qa/protocole.md) §7)
- **Qualité** (écart VMAF 4K vs Lanczos, en part de la marge 100 - VMAF Lanczos) : Qualité max >= 50 %, Équilibré >= 35 %, Rapide >= 20 % ; SSIM/PSNR : PSNR +3,0 / +1,9 / +1,0 dB (décision n° 24). Pas de régression > 5 % vs `main`.
- **Vitesse** (i/s de sortie, médiane de 5 passes) : Qualité max >= 30, Équilibré >= 60, Rapide >= 100.
- **VRAM** : avertissement > 3 Gio, FAIL > 9 216 Mio ; dérive sur 10 fichiers <= 50 Mio.
- Référence à battre : NVIDIA Broadcast 2.3 (VMAF >= Broadcast - 0,5).

## Blocages et points ouverts
- Sprint 2 : validation du contenu par le PO ; question PO n° 1 (sprint sans story `[UI]`) : [questions-po.md](questions-po.md).
- Poids Real-ESRGAN : licence à valider par secdevops avant usage (ENH-053).
- SEC-35 doit précéder ENH-030 ; suites LOW SEC-37 à SEC-40 : [ENH-047](stories/ENH-047-suites-secdevops.md).

## Où trouver quoi
- Décisions : [decisions.md](decisions.md) · Backlog (index + plan) : [backlog.md](backlog.md) · Vision : [vision.md](vision.md)
- Architecture : [architecture/index.md](architecture/index.md) (42 ADR rédigés) · Pipeline GPU : [gpu/architecture-pipeline.md](gpu/architecture-pipeline.md), [gpu/spike.md](gpu/spike.md)
- Sécurité : [security/threat-model.md](security/threat-model.md), [security/licences.md](security/licences.md), [security/zero-reseau.md](security/zero-reseau.md), [security/tracabilite-findings.md](security/tracabilite-findings.md)
- Design : [design/ui/design-system.md](design/ui/design-system.md), [design/ux/parcours.md](design/ux/parcours.md), [design/ux/criteres-transverses.md](design/ux/criteres-transverses.md)
- QA : [qa/protocole.md](qa/protocole.md), [qa/clips.md](qa/clips.md) · Revues : [reviews/](reviews/) · Sprints : [sprints/](sprints/) · Archives : [_archives/](_archives/)
