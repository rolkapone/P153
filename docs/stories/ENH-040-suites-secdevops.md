> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../backlog.md).

### Suites secdevops d'ENH-040 (ajoutées le 2026-10-24, portées par ENH-041 sauf mention)
- [ ] **SEC-30 (MEDIUM)** : si la création du Job Object échoue, **refuser l'effet** (erreur explicite) au lieu de continuer sans confinement ; test avec un faux échec.
- [ ] **SEC-31 (MEDIUM)** : le magasin `%ProgramData%\NVIDIA\NGX\models` est modifiable par tout utilisateur -> **contrôle avant `rvb_vsr_create`** (ACL / propriétaire ; refus avec `ComponentMissing` ou équivalent si non conforme) + **signalement à NVIDIA** (texte prêt pour le PO).
- [ ] **SEC-32** : figer la liste des **27 exports** du pont dans le test (échec si un export apparaît ou disparaît).
- [ ] **SEC-34** : mesure **M2 (pare-feu)** de `zero-reseau.md` rejouée sur le pont.
- [ ] **SEC-35** : `cuda.lib` de CUDA 13 embarque du code -> charger `nvcuda.dll` par `LoadLibraryExW` depuis System32 + `GetProcAddress`, **avant ENH-030** (rattaché à ENH-030 si ENH-041 est plein).
- [ ] **SEC-36** : corriger la note `docs/gpu/ENH-040.md` § 4 / l. 88 (périmée).
- [ ] **Dette `tidy.ps1`** : 26 avertissements `clang-tidy` (code antérieur au pont), à résorber (hors points, ou ENH-049).
Estimation : ces suites peuvent faire passer ENH-041 de 3 à 5 pts ; à confirmer à son planning.

Notes : secdevops + gpu-engineer ; dépend d'ENH-040 ; prépare ENH-028.
