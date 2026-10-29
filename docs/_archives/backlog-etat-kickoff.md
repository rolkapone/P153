> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../backlog.md).

## État des stories du kickoff

| Story | Statut | Reste à faire / transféré à |
|---|---|---|
| ENH-001 Spike VSR | **DONE** (5/5) | Critères transférés : sortie 4K lisible et VMAF/SSIM/PSNR de la VSR vs Lanczos -> **ENH-009** ; test réseau de NGX -> **ENH-041**. Livrable : `docs/gpu/spike.md` (phases 1-2) |
| ENH-002 Spike interpolation | **DONE** (5/5) | Brique choisie : RIFE 4.25 (ADR-043/044). Livrable : `docs/gpu/spike.md` (phase 3). Licences consignées. Suites : ENH-043, 044, 045, 046 |
| ENH-003 Repo, build, CI, tests GPU | **DONE** (4 pts, 2026-10-24) | `build.ps1` MSVC v145 + vcpkg, `test-gpu.ps1` (rapport 2026-10-21_1832 VERT), nvcc 13.4 sans `-allow-unsupported-compiler`, identité noreply locale + hook `commit-msg`, `ci.yml` et `security.yml`. **CI verte sur GitHub** le 2026-10-24 (commit `9a78fca` : `ci` vert, `security` 6/6, run 40038519042). Les durcissements devops restent portés par **ENH-049** |
| ENH-005 Protocole QA et clips | **DONE** (3/3) | Critère « extraits des archives du PO » -> **ENH-029** ; comparaison Broadcast en cours (renvoi ci-dessus) |
| ENH-006 Threat model et licences | **DONE** (3/3) | Findings SEC-01 à SEC-22 et registre livrés ; les actions sont reprises dans les stories ci-dessous |
| ENH-007 Design system | **DONE** (3/3) | Tokens (`tokens.json`, `tokens.css`, `Theme.qml`, `qmldir`), maquettes validées par le PO, contrastes vérifiés. Reste : ligne « icônes Lucide ISC » du registre (tâche secdevops, Sprint 1) |
