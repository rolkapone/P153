> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../backlog.md).

### Mouvements v0.5 (décision PO n° 20, 2026-10-24, après la QA d'ENH-040)

- **ENH-040 validée sur son périmètre (le pont)** : rapport `docs/qa/resultats/2026-10-24_ENH-040.md` conforme ; elle passe à l'étape sécurité. Le critère « qualité d'image VSR >= part Équilibré » est **retiré d'ENH-040 et déplacé dans ENH-009** (mesure q4 vs Lanczos : +13 % de part de marge en moyenne, BBB-FAST sous Lanczos, cible Équilibré 35 %).
- **ENH-053 créée (5 pts, Must, MVP, `[GPU]`)** : spike qualité de la super-résolution (VSR niveaux 1 à 4 vs Real-ESRGAN via TensorRT sur vidéos compressées). **ENH-009 en dépend** ; ENH-053 se place avant ENH-009 (Sprint 2 à recaler : ENH-053 + ENH-042 + ENH-009 dépassent 15 pts avec ENH-010, qui glisse alors au Sprint 3). **Total MVP +5 pts.**
- **Seuil de débit 300 i/s en q4 8 bits ramené à un avertissement** (le goulot de la chaîne est NVENC, environ 105 i/s) ; le seuil 10 bits >= 300 i/s reste bloquant (mesuré : 8 bits 290 i/s médiane, 10 bits 305).


### Mouvements v0.4 (planning du Sprint 1, 2026-10-21)

- **ENH-008 reste à 5 pts (Sprint 1)** mais son périmètre de matrice est réduit à la matrice « noyau » (8 combinaisons + ProRes en repli logiciel + clip QA `BBB-FAST`) : estimation du gpu-engineer = 8 pts pour le périmètre complet (`docs/gpu/ENH-008.md` § 8).
- **ENH-008b créée (3 pts, Must)** : reste de la matrice valide, 8 / 10 bits, VFR, rotation, dimensions impaires. À livrer **avant ENH-048** (qui rejoue la matrice) ; Sprint 2 reste à 15 pts, ENH-008b est placée au plus tard au Sprint 3. **Total MVP +3 pts.**
- Critères UX et seuils QA du Sprint 1 intégrés à ENH-004, 008, 008b, 040, 047 (`docs/design/ux/ENH-004.md`, `docs/qa/sprint-1-seuils.md`, `docs/qa/checklist-ui.md`).


### Mouvements v0.3 (traçabilité)

- ENH-009 8 -> 5 et ENH-012 8 -> 5 : le pont, les conversions couleur, le moteur TensorRT et la détection de coupure sont sortis en stories dédiées.
- ENH-015 3 -> 5 (limites, fuzzing, SEC-09, SEC-13), ENH-022 3 -> 5 (persistance durcie SEC-10/11/18), ENH-024 2 -> 3 (SEC-11), ENH-028 2 -> 3 (test dynamique, SEC-22), ENH-030 3 -> 5 (SEC-02/04/16, `package.ps1`).
- ENH-003 5 -> 4 pts : le reliquat (CI GitHub non exécutée, durcissements devops) est porté par ENH-049. **2026-10-24 : ENH-003 DONE, reste à 4 pts** (le reliquat n'était pas uniquement « CI non exécutée » : les durcissements devops restent dans ENH-049) ; aucun mouvement de points.
- ENH-001, ENH-002, ENH-005, ENH-006, ENH-007 : DONE ; leurs critères non remplis sont **transférés** à une story nommée (aucun critère perdu).
