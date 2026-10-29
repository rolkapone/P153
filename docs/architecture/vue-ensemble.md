> Extrait de l'ancien `docs/architecture.md` (ménage docs du 2026-10-29), texte inchangé. Index : [architecture/index.md](index.md).

## 1. Vue d'ensemble en couches

```
┌──────────────────────────────────────────────────────────────────────────┐
│ 1. UI — QML (Qt Quick)                                    thread UI     │
│    Main.qml, cartes, panneau Préréglage, feuilles, bandeaux             │
│    Theme (singleton du ui-designer : couleurs, typo, durées, courbes)   │
│    Affiche et anime. AUCUNE décision métier. Textes via qsTr().         │
└───────────────▲──────────────────────────────────┬───────────────────────┘
                │ Q_PROPERTY / NOTIFY, rôles        │ Q_INVOKABLE (intentions :
                │ du modèle, signaux                │ ajouter, démarrer, pause…)
┌───────────────┴──────────────────────────────────▼───────────────────────┐
│ 2. Présentation — C++20 + Qt (src/app)                    thread UI     │
│    QueueModel (QAbstractListModel)   PresetModel (QAbstractListModel)   │
│    AppController (état global, Démarrer/Pause, flux « Quitter »)        │
│    SettingsController  PrerequisitesController  NotificationModel       │
│    SystemAppearance (sombre/clair, contraste élevé, animations réduites)│
│    Traduction : codes d'erreur et valeurs -> textes FR (tr, QLocale)    │
│    Adaptateurs : QtExecutor (repost vers le thread UI), QtTimer         │
└───────────────▲──────────────────────────────────┬───────────────────────┘
                │ événements (structs C++ purs)     │ appels de méthodes
┌───────────────┴──────────────────────────────────▼───────────────────────┐
│ 3. Cœur métier — C++20 pur, SANS Qt (src/core)            thread UI     │
│    QueueService (machine à états des jobs, ordre, annuler/rétablir)     │
│    Orchestrator (un job à la fois, enchaînement, prévérifications)      │
│    PresetCatalog (préréglages en données JSON versionnées)              │
│    SettingsStore / QueueStore (persistance JSON atomique)               │
│    OutputPathPlanner (nommage, non-écrasement, fichiers temporaires)    │
│    EtaEstimator (temps restant lissé)  PrereqEvaluator (règles)         │
│    Log (façade spdlog partagée avec le pipeline)                        │
│    Ports (interfaces) : IExecutor, ITimer, IClock, IFileSystemInfo,     │
│                         IGpuInfoSource, IPipeline, IMediaProbe          │
└───────────────▲──────────────────────────────────┬───────────────────────┘
                │ rappels (threads du pipeline)     │ JobSpec, start/pause/
                │ -> repostés vers le thread UI     │ resume/cancel
┌───────────────┴──────────────────────────────────▼───────────────────────┐
│ 4. Interface pipeline — src/pipeline/include (gpu-engineer)             │
│    IPipeline, IMediaProbe, capacités GPU, PipelineError, FakePipeline   │
│    Implémentation : FFmpeg + NVDEC + CUDA + RTX Video SDK + NVENC       │
│    Threads propres au pipeline ; aucune dépendance à Qt                 │
└──────────────────────────────────────────────────────────────────────────┘
   Plateforme Windows (src/platform/win, sans Qt) : dossiers connus, DWM
   (Mica, barre de titre sombre), barre des tâches (progression), Explorateur
   (« Afficher dans le dossier »), contraste élevé, animations réduites.
```

Règles de dépendance (vérifiées en revue, et par CMake : une cible ne lie que la couche du dessous) :
- `enh_core` ne connaît ni Qt ni CUDA/FFmpeg ; il ne dépend que de la STL, de spdlog, de nlohmann/json, de tl::expected et des **interfaces** du pipeline.
- `enh_app` (présentation) dépend de `enh_core` et de Qt ; il ne parle jamais directement au pipeline concret.
- Le QML ne voit que les objets exposés par `enh_app`.
- `main.cpp` est la **racine de composition** : c'est le seul endroit qui instancie le pipeline réel (ou le faux) et le branche au cœur.

Arborescence proposée :

```
src/
  core/          enh_core (bibliothèque statique, sans Qt)
  platform/win/  enh_platform (Win32/COM, sans Qt)
  pipeline/      interface publique + implémentation (gpu-engineer)
  app/           enh_app : modèles, contrôleurs, adaptateurs Qt
  app/qml/       module QML « Enhance.App » (+ Theme du ui-designer)
  app/i18n/      enhance_fr.ts
  main.cpp
tests/unit/core/   tests/unit/app/   tests/unit/qml/   tests/gpu/
```

---
