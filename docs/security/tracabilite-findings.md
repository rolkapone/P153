> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../backlog.md).

## Traçabilité sécurité : findings -> stories

| Finding | Sév. | Traité par |
|---|---|---|
| SEC-01 NGX Update Module | HIGH | ENH-041 |
| SEC-02 TensorRT (obligations de livraison) | LOW (levé) | ENH-030 |
| SEC-03 poids RIFE / export maison | MEDIUM | ENH-043 (+ ENH-030) |
| SEC-04 FRUC non redistribuable | MEDIUM | ENH-030 (refus par `package.ps1`) ; FRUC reste repli local |
| SEC-05 config git globale | HIGH (processus) | **Clos** (PO + secdevops, 2026-10-24) |
| SEC-06 FFmpeg minimal | HIGH (porte de release) | ENH-048 (avant ENH-030) |
| SEC-07 CRL / OCSP de NGX | MEDIUM | ENH-041 |
| SEC-08 durcissement du chargement de DLL | MEDIUM | ENH-004 (+ ENH-040 pour les SDK) |
| SEC-09 sondage dans le processus principal | MEDIUM | ENH-015 (limites, fuzzing) ; isolement en processus = ENH-051 (Plus tard) |
| SEC-10 chemins UNC au redémarrage | MEDIUM | ENH-022 |
| SEC-11 suppression de temporaires | MEDIUM | ENH-022, ENH-024 |
| SEC-12 cache de moteurs TensorRT | MEDIUM | ENH-044 |
| SEC-13 dépassements d'entiers | MEDIUM | ENH-008 (décodeur, **fait**), ENH-015 (fin) |
| SEC-14 journaux tiers / WER | MEDIUM | ENH-025 |
| SEC-15 glisser-déposer, `ShellExecute` | LOW | ENH-017, ENH-023 |
| SEC-16 chemins de build dans les binaires | LOW | ENH-030 |
| SEC-17 `ffmpeg.lock.json` versionné | LOW | **fait** (`deps/ffmpeg.lock.json`) |
| SEC-18 bornes JSON | LOW | ENH-018, ENH-022 |
| SEC-19 binaires non signés | LOW | Avant publication (ENH-052) |
| SEC-20 exécution en administrateur | LOW | ENH-004 |
| SEC-21 CLUF d'Enhance | à confirmer | ENH-050 (+ ENH-052) |
| SEC-22 imports `ws2_32` de Qt6Core | LOW | ENH-028 |
| SEC-23 liste blanche de décodeurs, `format_whitelist`, `max_pixels` | MEDIUM | ENH-015 (+ liste de ENH-048) |
| SEC-27 cache de pipeline Qt (doc utilisateur, désinstalleur) | LOW | ENH-025, ENH-030 |
| SEC-28 `SetSearchPathMode` + test | LOW | Avant ENH-016 |

---
