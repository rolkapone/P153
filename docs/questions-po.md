> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](backlog.md).

## Questions pour le PO et points ouverts

**Pour le PO**
1. **Composition du Sprint 2** (2026-10-25) : valider « ENH-053 (spike qualité SR) + ENH-042 (conversions couleur) + ENH-041 (NGX hors ligne, sécurité) » = 15 pts, **sans écran à voir** ; ou préférer la variante « ENH-042 + ENH-009 + ENH-008b » (13 pts, déconseillée tant que le spike n'a pas tranché le moteur). Conséquence : J2 (premier fichier 4K) passe au Sprint 3.
2. **Real-ESRGAN (ENH-053)** : outil local Python approuvé (n° 12) ; confirmer que le PO accepte que l'équipe télécharge les poids Real-ESRGAN (licence à valider par secdevops avant) comme pour RIFE (n° 22).

(Les questions n° 1 à 4 de la v0.3 ont été tranchées le 2026-10-24 : décisions n° 22 à 25.)

**Résolues (historique)**
- *Résolu le 2026-10-24* — **Poids RIFE** (décision n° 22) : l'équipe télécharge `flownet.pkl` (Practical-RIFE, MIT) au sprint 3 ; SHA-256 consigné, README + `LICENSE` archivés, jamais commités (ENH-043).
- *Résolu le 2026-10-24* — **WER / SEC-14** (décision n° 23) : Enhance exclu des rapports d'erreurs Windows, aucun vidage ni envoi ; ligne de journal local avec code d'erreur seul (ENH-025).
- *Résolu le 2026-10-24* — **Seuils SSIM / PSNR** (décision n° 24) : validés (voir « Seuils QA de référence »).
- *Résolu le 2026-10-24* — **Notification de fin de file** (décision n° 25) : clignotement de l'icône + progression sur l'icône + résumé dans l'app + son discret désactivable ; toast avec l'installeur (ENH-019, ENH-033).
- *Résolu le 2026-10-21* — **Composition du Sprint 1** : validée par le PO (« Je valide le Sprint 1 avec les choix par défaut ») ; l'export RIFE (ENH-043) reste au Sprint 3.
- *Résolu le 2026-10-23* — **Squelette ENH-004 (UX)** : barre d'état en dur, capsule « Tester le ressort » visible, motif de test sur demande, icône provisoire (décisions consignées au sprint) ; ENH-004 livrée et vérifiée à l'écran par le PO le 2026-10-23.
- *Résolu le 2026-10-24* — **Dépôt GitHub** : `rolkapone/P153` créé, `main` poussée, CI verte (commit `9a78fca`) ; ENH-003 DONE.
- *Résolu le 2026-10-24* — **SEC-05** (config git globale) : adresse globale corrigée par le PO (noreply) ; finding clos par secdevops.

**Entre agents (consignés, pas pour le PO)**
- *Résolu* — **Pilote minimal affiché** : tranché par ADR-052 et aligné dans `parcours.md`.
- *Résolu* — **`parcours.md` P-06** (VRAM conservée à la pause, décision n° 8) : corrigé.
- **Ctrl+Entrée** et **Rétablir = relancer depuis le début** (`design-system-mouvement-composants.md` §11) : trancher ux / ui / gpu-engineer avant ENH-017 / ENH-021.
- **Textes P-08 `SourceUnreadable` et P-08b (préparation des modèles)** : **faits** par l'ux-designer (`parcours.md`), plus à transmettre.
- *Résolu* — **Comparaison Broadcast** : faite, rapport `docs/qa/resultats/2026-10-21_reference-broadcast.md` (le seuil « vs Broadcast » de ENH-009 / ENH-029 s'appuie dessus).
