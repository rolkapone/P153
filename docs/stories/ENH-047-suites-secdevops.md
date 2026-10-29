> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../backlog.md).

### Suites secdevops d'ENH-047 (ajoutées le 2026-10-25, LOW, rapport `docs/security/sprint-1.md`)
- [ ] **SEC-37** : `enhance-bench` n'appelle ni `SetDefaultDllDirectories` ni `SetDllDirectoryW(L"")` dans `wmain` -> appeler le durcissement d'`enh_platform` ; **à traiter avec la première story qui touche au banc (ENH-010)**.
- [ ] **SEC-38** : `--force` détruit l'ancienne sortie même si la mesure échoue -> fichier temporaire + `MoveFileExW(REPLACE_EXISTING)` ; à appliquer surtout aux sorties de l'application (ENH-010, ENH-024).
- [ ] **SEC-39** : le masquage des chemins (`redactPaths`) est sensible à la casse et limité aux chemins passés en argument -> insensible à la casse + `%TEMP%` / `%USERPROFILE%` (avec SEC-37 / SEC-38).
- [ ] **SEC-40** : rien n'empêche d'embarquer `enhance-bench.exe` et les DLL de développement voisines -> **dans ENH-030** : liste explicite des fichiers livrés + `forbiddenInPackage` dans `check-no-network.ps1 -Package`.
