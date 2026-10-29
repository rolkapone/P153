> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../backlog.md).

### Proposition de contenu du Sprint 2 (2026-10-25, scrum-master — non démarré, en attente du PO)

Objectif proposé : **« Choisir le bon moteur d'agrandissement, convertir les couleurs sur le GPU et prouver que la super-résolution ne parle pas au réseau. »**

| Story | Titre | Tags | Pts | Pourquoi dans ce sprint |
|---|---|---|---|---|
| ENH-053 | Spike qualité de la super-résolution (VSR q1-q4 vs Real-ESRGAN TensorRT) | `[GPU]` | 5 | Levier n° 1 sur la valeur du produit : la VSR n'apporte que +13 % de la marge vs Lanczos ; ENH-009 en dépend (décision n° 20). Poids Real-ESRGAN : licence à valider par secdevops avant usage |
| ENH-042 | Conversions couleur CUDA (NV12 / P010 <-> RGB) | `[GPU]` | 5 | Maillon manquant entre le décodeur (NV12 / P010) et la VSR (RGB) ; précède ENH-009 et ENH-010 ; testable en CI avec une référence CPU |
| ENH-041 | NGX hors ligne : neutraliser la mise à jour et le prouver (+ SEC-30 à SEC-36) | `[GPU]` | 5 (3 + suites secdevops, estimation à confirmer au planning) | Tient la promesse « 100 % local » tant que le pont est tout neuf (SEC-01 HIGH atténué) ; SEC-35 doit précéder ENH-030 |
| | **Total** | | **15** | |

- **Non retenues** : **ENH-008b** (3 pts, avant ENH-048, au plus tard Sprint 3) ; **ENH-009 et ENH-010** (dépendent d'ENH-053 / ENH-042) -> Sprint 3 ; l'ensemble 053 + 042 + 009 + 010 dépasse 15 pts. **Conséquence : J2 (premier fichier 4K de bout en bout) passe du Sprint 2 au Sprint 3.**
- Variante si le PO préfère un résultat plus visible : ENH-042 (5) + ENH-009 (5) sans attendre le spike, + ENH-008b (3) = 13 pts ; **déconseillée** : on implémenterait la VSR avant de savoir si c'est le bon moteur.
- Si ENH-041 reste à 3 pts après estimation : ajouter ENH-008b ne rentre pas (16 pts) ; compléter par les suites LOW SEC-37 / SEC-38 (hors points).
- Pas de story `[UI]` dans ce sprint : le PO ne verra pas de nouvelle fenêtre, mais **des images 4K comparées** (spike) et des rapports chiffrés ; à lui de dire si cela lui convient (question au PO n° 1).
