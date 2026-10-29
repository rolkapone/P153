> Extrait de l'ancien `docs/architecture.md` (ménage docs du 2026-10-29), texte inchangé. Index : [architecture/index.md](index.md).

## 14. Stratégie de tests sans GPU

| Niveau | Outil | Ce qui est testé | Où |
|---|---|---|---|
| Cœur métier | GoogleTest (BSD-3-Clause) | Machine à états (tous les chemins, ≥ 90 %), file (ajout, doublon, retrait + annulation, déplacement interdit du job en cours), orchestrateur avec `FakePipeline` (ordre, échec non bloquant, annulation, pause), `EtaEstimator`, `PresetCatalog` (validation, migrations, fichiers corrompus), `OutputPathPlanner` (accents, espaces, > 260 caractères, UNC, collisions), persistance (écriture atomique, fichier tronqué), `PrereqEvaluator` | `tests/unit/core`, CI |
| Déterminisme | Fausses implémentations des ports : `ManualExecutor`, `ManualTimer`/`FakeClock`, `FakeGpuInfoSource`, `FakeMediaProbe` | Pas de `sleep`, pas d'aléa ; le temps avance quand le test le décide | idem |
| Présentation Qt | Qt Test + **`QAbstractItemModelTester`** (mode fatal) + `QSignalSpy` | Cohérence des modèles (`QueueModel`, `PresetModel`, `NotificationModel`) pendant ajouts, déplacements, retraits, mises à jour de progression ; propriétés et signaux des contrôleurs ; `Formatter` français ; flux « Quitter » | `tests/unit/app`, CI, `QT_QPA_PLATFORM=offscreen` |
| QML | **Qt Quick Test** (`quick_test_main`) avec contrôleurs factices injectés | Liaisons et états visuels (carte dans ses 7 états), navigation clavier et ordre de tabulation, raccourcis, propriétés `Accessible`, durées à 0 en « animations réduites » ; aucune logique métier à tester côté QML | `tests/unit/qml`, CI (rendu logiciel WARP ou `offscreen`, à valider par devops) |
| Visuel (local) | Captures `QQuickWindow::grabWindow` à 100 % et 200 %, comparées aux références du ui-designer | Régressions de rendu, troncatures | local, QA |
| Accessibilité (manuel) | Narrateur, Accessibility Insights for Windows, mode contraste élevé | Noms, rôles, annonces, contrastes | QA |
| Fluidité (local, GPU) | Temps de trame Qt Quick (`QSG_RENDER_TIMING`) pendant un traitement 4K réel | ≥ 60 i/s d'interface (ENH-020) | `tests/gpu`, QA |

---
