> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../../backlog.md).

## Critères UX transverses (toute story `[UI]`, d'après `principes.md`)

Applicables en plus des critères propres à chaque story ; vérifiés par le ui-designer (contrôle design) et le qa-tester (`docs/qa/checklist-ui.md`).
- [ ] Parcours réalisable **au clavier seul** (ordre de tabulation = ordre visuel, focus visible >= 2 px et >= 3:1, pas de piège clavier, Échap restitue le focus).
- [ ] Aucun texte tronqué à **200 %** et à **760 px** de large ; rendu net à 100, 125, 150, 200 %.
- [ ] Contraste >= 4,5:1 (texte) et >= 3:1 (composants, focus), thèmes **sombre et clair** ; mode **contraste élevé Windows** : couleurs système, bordures visibles.
- [ ] **Narrateur** : nom, rôle, valeur de chaque contrôle ; lecture complète d'une carte dans chacun de ses 7 états ; progression annoncée <= 1 fois / 5 s ; erreurs en assertif.
- [ ] **« Réduire les animations »** de Windows : aucune transition de mouvement spatial ; aucune animation clignotante > 3 Hz.
- [ ] Textes en français (vouvoiement discret / formes impersonnelles, `qsTr`), nombres à la française ; un seul bouton plein accent à l'écran.
- [ ] Animations 150-350 ms issues de `Theme` (aucune durée, couleur ou taille en dur dans le QML) ; **fil d'interface : 99e centile des images <= 16,7 ms** pendant un traitement GPU (protocole §9).
- [ ] Captures de chaque état, thème et échelle fournies au ui-designer (sans chemin ni nom d'utilisateur visible) ; écart aux maquettes `docs/design/ui/maquettes/` classé CONFORME / ECARTS.
