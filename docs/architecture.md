# Enhance — Architecture

Statut : v0.1 du 2026-10-21 — **proposition** (developer), à valider par le PO (ADR-001 en particulier).
Périmètre de ce document : l'application **hors pipeline vidéo GPU** (interface, file d'attente, préréglages, persistance, journal, prérequis, fichiers). Le pipeline est décrit par le gpu-engineer dans `docs/gpu/architecture-pipeline.md` ; build, CI et packaging par devops (section dédiée en fin de document).

Numérotation des ADR, pour éviter les collisions entre auteurs :
- **ADR-001 à ADR-019** : application (developer) ;
- **ADR-020 à ADR-039** : build, dépendances, CI, packaging (devops) ;
- **ADR-040 et plus** : pipeline vidéo (gpu-engineer), y compris s'ils sont rédigés dans `docs/gpu/`.

Convention : **[H]** = hypothèse à confirmer (spike, prototype ou mesure).

---

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

## 2. ADR-001 — Technologie du front : Qt 6 / Qt Quick (QML)

### Contexte
Le PO impose un backend C++20 et laisse le front libre, sous trois conditions : finition « qualité Apple » (bloquante), aucune logique métier dans le front, application 100 % locale. Contraintes en plus : 60 images/s d'animation pendant un traitement GPU intensif, accessibilité (Narrateur, clavier, contraste élevé, 100–200 %), CI sans GPU, PO qui apprend le C++.

### Options comparées
Notes de 1 (mauvais) à 5 (excellent), avec un poids par critère.

| Critère (poids) | Qt 6 Quick (QML) | WinUI 3 (C++/WinRT) | HTML/CSS dans WebView2 | Dear ImGui |
|---|---|---|---|---|
| **Finition Apple** (×3) : animations 60 i/s, ressorts, flou, typo, HiDPI, sombre/clair | **4** — mise en forme libre, `SpringAnimation`, courbes de Bézier identiques aux maquettes CSS, `MultiEffect` (flou, ombres), *Animators* exécutés sur le thread de rendu ; Mica possible via DWM mais à prototyper | **3** — Mica, acrylique et Segoe UI Variable natifs, très bonnes animations Composition ; mais l'ADN visuel est Fluent : obtenir une allure « macOS » suppose de réécrire tous les modèles de contrôles en XAML | **5** — les maquettes du ui-designer sont déjà en HTML/CSS ; ressorts, `backdrop-filter`, typographie : le web excelle | **2** — rendu immédiat, pas de mise en page de texte riche, animations à la main |
| **Accessibilité** (×2) : Narrateur, clavier, contraste élevé | **3** — pont UI Automation via `Accessible`, `Accessible.announce()` (Qt ≥ 6.8) pour les régions vives ; contraste élevé à câbler nous-mêmes | **5** — natif Windows, contraste élevé automatique | **4** — ARIA traduit en UI Automation par Chromium | **1** — pas d'UI Automation |
| **Intégration C++20** (×2) | **4** — C++ natif, CMake de première classe, `QAbstractListModel`, `Q_PROPERTY`, `QML_ELEMENT` ; le préprocesseur `moc` reste compatible C++20 | **2** — projections C++/WinRT, fichiers IDL/MIDL, compilateur XAML, MSBuild obligatoire (pas de CMake officiel) ; Microsoft investit surtout C# | **3** — hôte C++ simple, mais frontière en messages JSON ; risque réel de voir de la logique glisser en JavaScript | **5** — bibliothèque C++ pure |
| **Glisser-déposer de fichiers** (×1) | **5** — `DropArea` fournit les chemins locaux | **4** — `StorageItems`, correct | **3** — le web masque les chemins ; interception native nécessaire | **3** — `WM_DROPFILES` à la main |
| **Testabilité sans GPU, en CI** (×2) | **5** — Qt Test, `QAbstractItemModelTester`, Qt Quick Test, plateforme `offscreen` | **2** — tests d'UI via WinAppDriver (plus maintenu) ou Appium ; XAML difficile à tester en unitaire | **4** — Playwright côté web, mais deux mondes de test | **3** — moteur de test séparé, licence à vérifier |
| **Licence et zéro réseau** (×2) | **3** — LGPLv3 (obligations ci-dessous) ; `Qt6Qml` des binaires officiels importe `Qt6Network` : build Qt sans `qml-network` nécessaire pour ENH-028 | **5** — MIT, aucun réseau | **1** — le runtime Edge/Chromium embarque une pile réseau et ses services (mises à jour, SmartScreen) : la preuve « zéro réseau » (exigence ferme) devient très difficile | **5** — MIT |
| **Taille du livrable** (×1) | **3** — ≈ 40–60 Mio de DLL Qt **[H]** | **4** — runtime Windows App SDK, ≈ 30–60 Mio en autonome **[H]** | **5** — runtime préinstallé sous Windows 11 | **5** — < 2 Mio |
| **Apprentissage pour le PO** (×1) | **3** — QML (déclaratif, proche de JSON + JavaScript) et le modèle objet de Qt (`QObject`, parents, signaux) | **2** — XAML + C++/WinRT + notions COM | **3** — trois langages (HTML/CSS, JavaScript, C++) et un pont | **4** — C++ seul |
| **Total pondéré (/70)** | **53** | **47** | **50** | **46** |

### Décision
**Qt 6 / Qt Quick (QML)**, version **6.8 LTS minimum**, idéalement la dernière 6.x stable au moment d'ENH-004, épinglée (voir ADR devops pour la source des binaires). Raisons décisives :
1. Liberté visuelle complète pour reproduire les maquettes au pixel (pas d'ADN Fluent à combattre), avec des animations sur le thread de rendu qui restent fluides si le thread UI est momentanément chargé.
2. Intégration C++ et CMake de première classe ; la frontière « QML affiche, C++ décide » est naturelle (`Q_PROPERTY`, modèles, `Q_INVOKABLE`).
3. Seule option qui combine testabilité complète en CI sans GPU et contrôle de la preuve « zéro réseau ».

Pourquoi pas WebView2, pourtant le meilleur en finition : Chromium embarqué rend la garantie « aucun appel réseau » invérifiable de façon simple (exigence ferme du PO), coûte 150–300 Mio de RAM et plusieurs processus, et rend difficile l'accès aux chemins des fichiers déposés.
Pourquoi pas WinUI 3 : meilleure accessibilité et Mica natif, mais outillage C++ lourd (MSBuild, IDL), allure Fluent difficile à transformer en « macOS », tests d'interface faibles.
Pourquoi pas Dear ImGui : accessibilité inexistante, finition incompatible avec l'exigence n° 1.

### Comment atteindre la finition « Apple » sous Qt Quick
| Besoin | Moyen | Risque |
|---|---|---|
| Animations douces, ressorts | `NumberAnimation` avec `Easing.Bezier` (mêmes courbes que les tokens CSS des maquettes), `SpringAnimation`, transitions `add`/`move`/`displaced` de `ListView` (cascade de dépôt, réordonnancement) ; durées et courbes lues dans `Theme` | Faible |
| 60 i/s pendant le traitement | boucle de rendu `threaded` (Direct3D 11 via RHI) ; animations clés en *Animators* (`OpacityAnimator`, `ScaleAnimator`, `YAnimator`) qui tournent sur le thread de rendu ; rien de lourd sur le thread UI (ADR-002) ; progression regroupée (≤ 10 Hz) | Moyen : partage du GPU avec CUDA/NVENC, à mesurer par QA (ENH-020) |
| Mica / fond translucide de fenêtre | `DwmSetWindowAttribute(DWMWA_SYSTEMBACKDROP_TYPE, DWMSBT_MAINWINDOW)` (Windows 11 22H2+), `DwmExtendFrameIntoClientArea` (marges -1), fenêtre Qt Quick à fond transparent ; barre de titre sombre via `DWMWA_USE_IMMERSIVE_DARK_MODE`, couleur de légende via `DWMWA_CAPTION_COLOR`. Encapsulé dans `enh_platform` (classe RAII qui applique et restaure) | **Moyen–élevé** : transparence de fenêtre Qt Quick + D3D11 à prototyper dans ENH-004 ; repli = fond opaque teinté par les tokens, quasi indiscernable |
| Flou interne (feuilles au-dessus de la liste) | `MultiEffect` (`blurEnabled`) de `QtQuick.Effects` sur une capture de la liste, limité aux feuilles | Faible (coût GPU modeste, à mesurer) |
| Typographie | Police fournie par le design system (Segoe UI Variable, police système, ou Inter OFL embarquée via `FontLoader`) ; choix du mode de rendu du texte (`Text.NativeRendering` = DirectWrite, plus net à 100 % ; `QtRendering` = meilleur pendant les mises à l'échelle animées) arbitré par le ui-designer sur prototype | Faible |
| HiDPI 100–200 % | Qt 6 gère la mise à l'échelle par moniteur (manifeste « PerMonitorV2 », arrondi `PassThrough`) ; icônes vectorielles (SVG) ; tailles en pixels logiques issues de `Theme` | Faible |
| Sombre / clair / système | `SystemAppearance` (C++) lit `QStyleHints::colorScheme()` et le registre Windows ; `Theme` choisit la palette ; transition animée < 300 ms | Faible |
| Contraste élevé, animations réduites | `SystemParametersInfoW(SPI_GETHIGHCONTRAST)` + `GetSysColor` et `SPI_GETCLIENTAREAANIMATION`, exposés en `Q_PROPERTY` à `Theme` (durées à 0, couleurs système) | Faible |
| Contrôles | Qt Quick Controls style **Basic** (ou *Templates*) entièrement restylés avec les tokens ; pas de style « Windows » ni « FluentWinUI3 » (non personnalisables au niveau voulu) | Faible |

### Conséquences
- Tout l'état visible est porté par des objets C++ (`Q_PROPERTY` + `NOTIFY`, rôles de modèle) ; le QML ne fait que des liaisons et des animations. Une revue refuse tout `if` métier dans le QML (règles de cadence, nommage, choix de conteneur, etc.).
- QML compilé à la construction (`qt_add_qml_module`, `qmlcachegen`) pour tenir le démarrage < 2 s ; les DLL lourdes (FFmpeg, CUDA, SDK) sont chargées après l'affichage de la fenêtre, hors thread UI.
- Prototype obligatoire dans ENH-004 : fenêtre Mica + barre de titre sombre + une animation ressort, mesurés à 100 % et 200 %.
- **Licence Qt (LGPLv3, liaison dynamique)** — obligations : DLL Qt séparées et remplaçables par l'utilisateur (jamais de liaison statique) ; textes LGPLv3/GPLv3 livrés ; mention de Qt et de sa licence dans « À propos » ; mise à disposition des sources de Qt utilisées (ou offre écrite), **y compris la configuration de build** si Qt est recompilé (cas probable, voir ci-dessous) ; aucune clause de licence interdisant l'ingénierie inverse nécessaire pour remplacer Qt. Modules limités aux modules LGPL : QtCore, QtGui, QtQml, QtQuick, QtQuick.Controls (Basic/Templates), QtQuick.Effects, QtQuick.Dialogs, QtSvg, QtQuickTest (tests uniquement). Exclus : QtNetwork, modules GPL seulement (Charts, Quick 3D, Virtual Keyboard…). Les outils de build (ex. `qsb` de Qt Shader Tools) ne sont pas redistribués. À consigner par secdevops dans `docs/security/licences.md`.
- **Zéro réseau (ENH-028)** : dans les binaires Qt officiels, `Qt6Qml.dll` dépend de `Qt6Network.dll` (fonction `qml-network`). Pour livrer Qt sans module réseau, il faut un Qt compilé sans cette fonction (vcpkg avec options, ou build maison). Choix de la source de Qt = ADR devops (ADR-02x).
- Version : la politique Qt réserve les correctifs tardifs des LTS aux licences commerciales ; en open source on suit la dernière mineure stable ≥ 6.8, épinglée, et on la remonte au moins une fois par release.

---

## 3. ADR-002 — Couches, modèle de threads et passage vers le thread UI

### Contexte
L'interface doit rester à 60 i/s pendant un traitement GPU ; le PO apprend le C++ et doit pouvoir lire le cœur sans connaître Qt ; la CI n'a pas de GPU.

### Décision
- **Cœur sans Qt** (`enh_core`, C++20 standard), testé par GoogleTest.
- **Le cœur vit sur le thread UI, en mode « acteur à un seul thread »** : toutes ses méthodes sont appelées depuis ce thread, donc **aucun verrou** dans la logique métier. Ses opérations sont légères (liste de quelques dizaines de jobs).
- **Tout ce qui est lent tourne ailleurs** : sondage des fichiers (`IMediaProbe`, pool borné de 2–4 threads), pipeline (threads du gpu-engineer), écriture des fichiers JSON (thread d'écriture), journal (spdlog asynchrone), détection des prérequis.
- **Retour sur le thread UI** par un port `IExecutor` (« exécute cette fonction sur le thread du cœur ») : implémentation Qt = `QMetaObject::invokeMethod(contexte, fonction, Qt::QueuedConnection)` ; implémentation de test = file manuelle vidée par le test (déterministe).
- **Regroupement de la progression** : l'adaptateur du pipeline conserve le dernier échantillon dans une variable atomique et ne poste qu'**un** événement à la fois vers le thread UI (pas d'inondation de la boucle d'événements) ; le modèle n'émet `dataChanged` que pour les rôles de progression de la ligne concernée ; l'affichage est rafraîchi au plus toutes les 500 ms (ENH-020).
- **Annulation coopérative** par `std::stop_source` / `std::stop_token` (C++20) ; threads en `std::jthread` (rejoints automatiquement à la destruction).
- Temps : `std::chrono` ; minuteries (fenêtre « Rétablir » de 5 s, anti-rebond des sauvegardes) via un port `ITimer` (Qt = `QTimer`, test = horloge manuelle).

### Alternatives écartées
1. **Cœur sur un thread dédié avec verrous** : plus de parallélisme inutile, mutex partout, bugs de concurrence difficiles à tester ; rien dans le cœur n'est assez lourd pour le justifier.
2. **Cœur écrit avec Qt (`QObject`, `QThread`, `QJsonDocument`)** : plus rapide à écrire, mais lie la logique à Qt, complique les tests sans boucle d'événements et éloigne le PO du C++ standard.

### Conséquences
- Règle de revue : aucun appel bloquant (E/S disque, FFmpeg, CUDA, `join`) sur le thread UI.
- Les rappels du pipeline ne doivent jamais appeler le pipeline en retour ni bloquer (voir § 11).
- Licences : STL (MSVC), GoogleTest (BSD-3-Clause, tests uniquement).

---

## 4. ADR-003 — Gestion des erreurs : `expected` et aucune exception aux frontières

### Contexte
ENH-015 : aucune exception ne doit traverser l'API ; chaque erreur doit devenir un message français « quoi + que faire » (P-08). Le projet est en C++20 ; `std::expected` est une nouveauté de C++23.

### Décision
- Type d'erreur à valeurs : `enh::Expected<T, E>`, **alias de `tl::expected`** (bibliothèque d'un seul en-tête, interface identique à `std::expected`), à remplacer par `std::expected` le jour où le projet passe en C++23 (changement d'une ligne). **À partager avec le gpu-engineer** pour que l'interface pipeline utilise le même alias.
- Les erreurs sont des **codes énumérés + données** (ex. `SourceCorruptAt{instant}`, `DiskFull{lecteur, manque}`), jamais des phrases ; la présentation traduit en français. Le détail technique (anglais, pour « Copier le détail » et le journal) est un champ séparé.
- Les exceptions de la STL ou de bibliothèques tierces (ex. `nlohmann::json::parse_error`, `std::bad_alloc`) sont attrapées au bord du module qui les provoque et converties en `Expected`.

### Alternatives écartées
1. **Passer le projet en C++23** pour avoir `std::expected` : la stack imposée est C++20 ; à rediscuter avec le PO et devops plus tard.
2. **Codes de retour à la C (`int` + paramètre de sortie)** : faciles à ignorer, aucun lien entre résultat et erreur.

### Conséquences
Licence : tl::expected, CC0-1.0 (domaine public).

---

## 5. ADR-004 — File d'attente et orchestration des jobs

### Contexte
ENH-016, 019, 021, 022 : états testés à ≥ 90 % des chemins, un job à la fois, échec non bloquant, pause, annulation sans boîte de confirmation (« Rétablir » 5 s), réordonnancement, file persistée ; plus tard reprise après plantage (ENH-038) et dossiers entiers (ENH-037).

### Décision
- **Job** = identifiant (entier 64 bits croissant, persisté), chemin source (`std::filesystem::path`), `MediaInfo` (résultat du sondage), **instantané des paramètres de préréglage** figé au démarrage (les cartes déjà lancées gardent leur préréglage, P-05), chemins de sortie et temporaire, état, résultat.
- **Machine à états explicite**, fonction pure `transition(état, événement) -> Expected<état, TransitionError>` :

```
Lecture (sondage) ──ok──> En attente ──Démarrer──> En cours ──fin──> Terminé
      │ échec : non ajouté          ▲                 │  ▲ │
      │ (message « refusé »)        │            Pause│  │Reprendre
      ▼                             │                 ▼  │
   (retiré)                         │              En pause
                                    │                 │
              Réessayer / Relancer ─┤        Annuler  │  Annuler   erreur
                                    │                 ▼            ▼
                                 Annulé <──────────(En cours)   Échec
```
  (Annulé et Échec repassent « En attente » par Réessayer/Relancer ; Terminé, Échec, Annulé peuvent être retirés.)
- **QueueService** : ajout (avec détection de doublons sur chemin normalisé, insensible à la casse), retrait, déplacement (interdit pour le job en cours), sélection du préréglage pour les jobs non commencés. Les actions destructrices renvoient un **jeton d'annulation** ; elles sont appliquées visuellement tout de suite et **validées** à l'expiration du délai de 5 s (suppression effective du fichier partiel à ce moment-là) ; `undo(jeton)` rétablit. Délai = constante du cœur, minuterie via `ITimer`.
- **Orchestrator** : prend le premier job « En attente », exécute les prévérifications (espace disque estimé via `IPipeline::plan`, VRAM, prérequis), réserve le nom de sortie, lance le pipeline, consomme ses événements, finalise (renommage du temporaire), puis enchaîne. Un échec ou une annulation passe simplement au suivant.
- **Rétablir après annulation** = relancer **depuis le début** (le partiel n'est pas réutilisable au MVP) ; libellé « Reprendre depuis le début » à confirmer avec l'ux-designer.
- **Temps restant** : `EtaEstimator` (pur) — « Estimation en cours… » pendant les 10 premières secondes, puis moyenne glissante exponentielle des images/s ; ETA global = somme des estimations des jobs restants.
- **Exposition** : `QueueModel` (`QAbstractListModel`) avec des rôles nommés (`fileName`, `state`, `progress`, `fps`, `remaining`, `stage`, `badges`, `errorText`, `canMove`…), notifications fines (`rowsInserted`, `rowsMoved`, `dataChanged` limité aux rôles modifiés). Les miniatures passent par un `QQuickAsyncImageProvider` (génération hors thread UI).
- **Fermeture** (décision PO n° 6) : `AppController` intercepte la fermeture de fenêtre ; pendant un traitement, la croix (et Alt+F4) réduit la fenêtre dans la barre des tâches et le traitement continue ; Ctrl+Q expose `quitConfirmationRequested` → feuille QML → `confirmQuit()` → annulation propre (≤ 3 s), sauvegarde, sortie. Sans traitement, Ctrl+Q quitte directement. Fermeture de session Windows : sauvegarde de la file puis annulation propre.
- Bonus de finition proposé (à valider par l'ux-designer) : progression globale sur l'icône de la barre des tâches (`ITaskbarList3`, encapsulé RAII dans `enh_platform`).

### Alternatives écartées
1. **Logique de file dans le QML** (`ListModel` QML + JavaScript) : interdit par la règle de séparation, non testable en C++.
2. **Machine à états par bibliothèque (Boost.MSM, SML)** : puissante mais opaque pour un débutant ; sept états tiennent dans un tableau lisible.

### Conséquences
Notification de fin de file (ENH-019) : la notification Windows native (toast) exige pour une app non empaquetée un identifiant d'application (AUMID) et un raccourci du menu Démarrer, absents en version portable. Choix à faire dans ENH-019 entre toast WinRT (installeur) et repli discret dans l'app + clignotement de la barre des tâches. Licences : aucune brique nouvelle.

---

## 6. ADR-005 — Préréglages en données JSON versionnées

### Contexte
ENH-018 : définitions (modes SDK, débit, 10 bits, interpolation) dans un fichier versionné, pas dans le QML ; plus tard préréglages personnalisés (ENH-034) et options avancées (ENH-035) ; valeurs numériques fixées par les spikes GPU.

### Décision
- Un **schéma JSON** avec `schemaVersion`. Chaque préréglage : `id` stable (`quality`, `balanced`, `fast`), `builtin`, et des paramètres **par format** (`hevc`, `av1`) : mode de super-résolution, résolution cible (3840×2160), règle d'interpolation (cible 60 i/s, seuil 50 i/s, qualité normale ou dégradée — dégradée autorisée **seulement** pour `fast`, décision PO n° 5, contrôlée par la validation), profondeur 8/10 bits, débit cible en kbit/s, préréglage NVENC. Les valeurs numériques sont fournies par le gpu-engineer après les spikes **[H]**.
- Les préréglages intégrés sont **embarqués dans l'exécutable** (fichier `builtin_presets.json` versionné dans le dépôt, transformé en constante par CMake) : impossible à perdre ou à altérer. Les préréglages personnalisés (plus tard) iront dans `%LOCALAPPDATA%\Enhance\presets\`.
- Chargement → **validation** (plages, champs obligatoires, combinaisons interdites) → `PresetCatalog` immuable ; migration explicite `vN → vN+1` pour chaque version de schéma ; un fichier utilisateur invalide est ignoré et journalisé, jamais bloquant.
- Libellés et descriptions des préréglages intégrés : traduits côté présentation à partir de l'`id` (pas de texte dans le JSON) ; les préréglages personnalisés porteront le nom saisi.
- Un préréglage se traduit en `PipelineParams` (structure de l'interface pipeline) par une fonction pure testée.
- Exposé au QML par `PresetModel` (`QAbstractListModel`) + `Q_PROPERTY` `selectedPresetId`, `selectedFormat`, `av1Available`.

### Alternatives écartées
1. **Préréglages codés en dur en C++** : contraire à ENH-018, rend ENH-034/035 coûteux.
2. **`QSettings`/INI ou `QJsonDocument`** : introduit Qt dans le cœur ; INI mal adapté aux structures imbriquées.

### Conséquences
Licence : nlohmann/json, MIT.

---

## 7. ADR-006 — Persistance des réglages et de la file

### Contexte
Mémorisation silencieuse (préréglage, format, dossier, thème, ordre de la file), file persistée (ENH-022) et préparée pour la reprise après plantage (ENH-038) ; une coupure de courant pendant l'écriture ne doit jamais corrompre les données.

### Décision
- Emplacement : `%LOCALAPPDATA%\Enhance\` obtenu par `SHGetKnownFolderPath(FOLDERID_LocalAppData)` (dans `enh_platform`) :
  - `settings.json` : thème (`dark` | `light` | `system`), dossier de sortie (absent = à côté de la source), dernier préréglage, dernier format ;
  - `queue.json` : jobs (identifiant, chemin source, état, instantané des paramètres, chemins de sortie et temporaire, résultat) ; écrit à chaque **changement structurel** (ajout, retrait, déplacement, changement d'état), jamais à chaque échantillon de progression ;
  - `logs\` (ADR-007).
- **Écriture atomique** : écriture complète dans `fichier.tmp`, vidage disque, puis remplacement par `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` ; sauvegardes regroupées (anti-rebond 300 ms) et exécutées sur un thread d'écriture à partir d'un **instantané** copié.
- Tous les fichiers ont `schemaVersion` ; lecture tolérante (fichier absent ou corrompu → valeurs par défaut + avertissement au journal, l'ancien fichier est conservé en `.bak`).
- Au redémarrage (MVP) : un job trouvé « En cours » ou « En pause » repasse « En attente » (reprise depuis le début) et son fichier temporaire **connu** est supprimé. Le bandeau « Un traitement a été interrompu. Reprendre ? » appartient à ENH-038. *À confirmer avec l'ux-designer.*
- `queue.json` contient les chemins complets des vidéos (nécessaire au fonctionnement) ; il reste dans le profil de l'utilisateur et n'est jamais journalisé.

### Alternatives écartées
1. **`QSettings` (registre Windows)** : Qt dans le cœur, registre peu inspectable, mal adapté à une liste de jobs.
2. **SQLite** : robuste et transactionnel, mais dépendance supplémentaire disproportionnée pour deux petits fichiers ; à reconsidérer si la reprise après plantage exige des points de reprise fréquents.

### Conséquences
Licences : nlohmann/json (MIT) ; API Windows (système).

---

## 8. ADR-007 — Journalisation locale

### Contexte
ENH-025 : journal tournant 5 × 2 Mio dans `%LOCALAPPDATA%\Enhance\logs`, niveaux Info/Avertissement/Erreur, aucun chemin complet ni donnée personnelle ; le pipeline (FFmpeg, CUDA) doit écrire dans le même journal ; journaliser ne doit pas ralentir le thread UI.

### Décision
- **spdlog** en mode asynchrone, `rotating_file_sink` (2 Mio × 5 fichiers), formatage via `std::format` (option `SPDLOG_USE_STD_FORMAT`, ce qui évite la dépendance à fmt).
- Façade `enh::log` dans le cœur (fonctions `info`, `warn`, `error`, `debug`) utilisée par **toutes** les couches, y compris le pipeline (qui y redirige `av_log` de FFmpeg).
- Messages Qt redirigés via `qInstallMessageHandler`.
- Confidentialité : utilitaire `log::fileNameOnly(path)` imposé en revue ; un mode diagnostic (`--verbose`, désactivé par défaut) peut ajouter les chemins complets.
- Vidage du journal à la fermeture et sur erreur fatale ; « Afficher les journaux » ouvre le dossier dans l'Explorateur.

### Alternatives écartées
1. **`QLoggingCategory`** : Qt dans le cœur et le pipeline ; pas de rotation de fichiers intégrée.
2. **Boost.Log** : très complet mais lourd à compiler et à apprendre pour un besoin simple.

### Conséquences
Licence : spdlog, MIT.

---

## 9. ADR-008 — Internationalisation (français d'abord)

### Contexte
ENH-027 : aucune chaîne en dur, français d'abord, architecture prête pour d'autres langues ; nombres, durées et tailles à la française (« 1,2 Gio », « 3 min 05 s ») ; durées lues naturellement par le Narrateur.

### Décision
- Toutes les chaînes visibles via `qsTr()` (QML) et `tr()` (présentation C++) ; le **cœur ne produit aucun texte** destiné à l'utilisateur (codes + valeurs, voir ADR-003).
- Chaînes sources **en français** (langue de référence), fichiers `enhance_fr.ts` → `.qm` générés par `qt_add_translations` (CMake) et embarqués ; une deuxième langue = un nouveau `.ts`.
- Formatage centralisé dans un `Formatter` de la présentation (testé en unitaire) : `QLocale(fr_FR)` pour les nombres, règles maison pour les durées (« 3 min 05 s ») et tailles binaires (« Gio »), version longue pour le Narrateur (« 3 minutes 5 secondes »).
- Pluriels via `qsTr("%n vidéo(s)", "", n)`.

### Alternatives écartées
1. **gettext / Boost.Locale** : outillage hors Qt, double système alors que Qt Linguist s'intègre au QML.
2. **Chaînes sources en anglais + traduction française** : plus classique, mais la relecture des micro-textes par l'ux-designer se ferait sur une traduction ; le français est la langue de référence du produit.

### Conséquences
Licence : Qt Linguist et outils de traduction (LGPL/GPL, outils de build non redistribués).

---

## 10. ADR-009 — Détection des prérequis (carte, pilote)

### Contexte
ENH-031 : au lancement, détecter la carte (RTX 50 attendue) et la version du pilote, message français avec la version minimale ; carte non compatible → lecture seule, sans plantage ; logique testée sans GPU. ENH-011 : griser AV1 si indisponible.

### Décision
- Règles **pures** dans le cœur : `PrereqEvaluator::evaluate(GpuInfo, Requirements) -> PrereqStatus` (`Ok`, `DriverTooOld{trouvée, requise}`, `IncompatibleGpu{nom}`, `NoNvidiaGpu`, `ComponentMissing{détail}`). Les exigences (pilote minimal, capacité de calcul ≥ 12.0, composants requis) sont des **données** mises à jour après les spikes.
- Source des informations : port `IGpuInfoSource`. Implémentation principale = **capacités exposées par l'interface pipeline** (nom, pilote, capacité de calcul, VRAM, NVENC HEVC/AV1 10 bits, présence des DLL du SDK), car le gpu-engineer maîtrise NVML et le pilote. Repli : énumération DXGI (identifiant fabricant NVIDIA `0x10DE`) pour nommer la carte quand le pilote NVIDIA est absent.
- Sécurité : DLL du pilote chargées uniquement depuis `System32` (`SetDefaultDllDirectories` + `LoadLibraryExW(..., LOAD_LIBRARY_SEARCH_SYSTEM32)`), pour éviter le détournement de DLL.
- Exécutée hors thread UI juste après l'affichage de la fenêtre (< 1 s) ; résultat exposé par `PrerequisitesController` (`Q_PROPERTY` `status`, `gpuName`, `canProcess`, `av1Available`) ; l'app reste explorable, seul « Démarrer » est désactivé.

### Alternatives écartées
1. **Appeler NVML directement depuis l'application** : doublon avec le pipeline et risque d'écart entre ce que voit l'interface et ce que le pipeline accepte.
2. **WMI (`Win32_VideoController`)** : lent (plusieurs centaines de ms), version de pilote au format Windows difficile à convertir.

### Conséquences
Licences : NVML est fourni par le pilote (non redistribué) ; DXGI fait partie de Windows.

---

## 11. ADR-010 — Fichiers, chemins et sortie

### Contexte
ENH-024 : sortie à côté de la source avec suffixe `_Enhance_4K60`, jamais d'écrasement, dossier unique en option, chemins accentués, avec espaces, > 260 caractères, lecteurs réseau ; espace disque vérifié avant le démarrage ; aucun fichier incomplet laissé sur le disque.

### Décision
- Chemins manipulés exclusivement en `std::filesystem::path` (UTF-16 sous Windows) ; conversion depuis Qt par `QString::toStdWString()` ; manifeste de l'application avec `longPathAware` ; passage au pipeline en `std::filesystem::path` (à charge du gpu-engineer de transmettre à FFmpeg une forme qui supporte les chemins longs, à vérifier ensemble).
- `OutputPathPlanner` (pur, testé) : nom final `<nom>_Enhance_4K60.<ext>` ; si le fichier existe ou est déjà réservé par un autre job de la file : `_2`, `_3`… ; extension imposée par le plan du pipeline (`.mp4` ou `.mkv` en cas de bascule).
- Le pipeline écrit dans un **temporaire** `<nom final>.enhance-tmp.<ext>` du même dossier ; en cas de succès, renommage par `MoveFileExW` **sans** remplacement (si le nom a été pris entre-temps : nom suivant) ; en cas d'échec ou d'annulation : suppression du temporaire (après le délai « Rétablir »). Au démarrage, seuls les temporaires **référencés dans `queue.json`** sont nettoyés (jamais de suppression par motif dans les dossiers de l'utilisateur).
- Espace disque : `std::filesystem::space` sur le volume de sortie, comparé à l'estimation de taille du plan.
- « Lire » : `ShellExecuteExW` ; « Afficher dans le dossier » : `SHOpenFolderAndSelectItems` ; enveloppes RAII pour les `PIDL` COM.

### Alternatives écartées
1. **Écrire directement le nom final** : un plantage laisse un fichier incomplet sous un nom trompeur.
2. **Temporaires dans `%TEMP%`** : le renommage final devient une copie entre volumes (lente, double espace disque).

### Conséquences
Cas de test unitaires : accents, espaces, chemins > 260 caractères, UNC `\\serveur\partage`, nom déjà existant, deux jobs du même nom vers le même dossier. Licences : aucune brique nouvelle.

---

## 12. Ce que l'application attend de l'interface pipeline

**Contrat final, tranché le 2026-10-21 entre developer et gpu-engineer (ADR-047, ADR-049, ADR-051).** Le gpu-engineer reste propriétaire de l'interface ; le code de référence (signatures complètes) est dans `docs/gpu/architecture-pipeline.md` § 6. Tout est sans Qt, utilisable et testable sans GPU grâce à **`FakePipeline`** / **`FakeMediaProbe`** (cible `enh_pipeline_fake` : vitesse, étapes, erreur injectée à l'image N, horloge pilotée par le test, déterministe). En-têtes publics : `src/pipeline/include/enh/pipeline/` ; alias d'erreur : `src/common/include/enh/expected.hpp` (cible `enh_common`, partagée).

Légende : ✔ = proposition de l'application retenue telle quelle ; ✎ = retenue avec précision ou modification (en gras).

| Élément | Contrat final | |
|---|---|---|
| `IMediaProbe::probe(chemin, stop_token)` | `Expected<MediaInfo, PipelineError>` : durée, taille d'affichage (rotation et SAR appliqués), cadence (fraction) et VFR, codec, profondeur de bits, nombre d'images estimé, entrelacement, HDR10, pistes audio / sous-titres / pièces jointes (avec compatibilité MP4). CPU seulement, `const`, appelable depuis plusieurs threads, < 1 s (**échéance dure de 2 s + interruption par `stop_token`**). | ✎ |
| `IMediaProbe::thumbnail(chemin, taille max, stop_token)` | `Expected<Thumbnail, PipelineError>` (RGBA 8 bits non prémultiplié, taille en pixels) ; CPU seulement. | ✔ |
| `IPipeline::capabilities()` | Nom de la carte, version du pilote, capacité de calcul, VRAM totale et libre (Mio), encodeurs (HEVC/AV1, 8/10 bits), codecs NVDEC, présence des composants (RTX Video SDK, TensorRT, modèle d'interpolation). < 1 s, hors thread UI. Sert ENH-031 et ENH-011. | ✔ |
| `plan(MediaInfo, PipelineParams, GpuCapabilities)` | **Fonction libre** pure, `noexcept`, sans E/S ni GPU (**prend `PipelineParams` et non `JobSpec`**, car le nom de sortie dépend du conteneur choisi par le plan ; **et un instantané `GpuCapabilities`**, simple donnée, pour AV1 et NVDEC) : interpolation si < 50 i/s → 60 i/s, agrandissement ignoré si la source atteint déjà la boîte cible, conteneur (MP4 ou bascule MKV + pistes concernées) et **extension** (`.mp4`/`.mkv`), taille estimée (octets), VRAM estimée (Mio), durée estimée (s, indicative), notes. | ✎ |
| `JobSpec` | Sérialisable (`schema_version`) : chemin source, **`output_path` = chemin exact à créer (le temporaire de l'application)**, conteneur (recopié du plan), `PipelineParams`, plage optionnelle (aperçu). | ✔ |
| `start(JobSpec, IJobObserver&)` | `Expected<JobHandle, PipelineError>` ; un seul job actif (`Busy` sinon) ; **non bloquant (< 50 ms)** : ouverture, VRAM, construction des modèles se font ensuite, leurs erreurs arrivent par `onFinished`. Si `start` échoue, aucun rappel. | ✎ |
| `JobHandle` | Classe valeur RAII **déplaçable, non copiable** ; destruction après `onFinished` immédiate, avant = annule et attend (≤ 3 s, à réserver à la fermeture). `wait()` pour les tests et la fermeture. | ✔ |
| `pause()` / `resume()` / `cancel()` | Non bloquants, thread-safe, idempotents ; pause effective < 2 s (`onStateChanged(Paused)`) **avec VRAM conservée au MVP** (ADR-046, à valider par le PO) ; annulation < 3 s, VRAM libérée, fichier fermé, puis `onFinished(Cancelled)`. | ✎ |
| Fichiers (ADR-010 / ADR-049) | **Tranché : l'application possède nommage, temporaire, renommage, suppression et nettoyage.** Le pipeline crée `output_path` en `CREATE_NEW` (existe déjà → `OutputExists`), écrit, et en cas de succès vide (`FlushFileBuffers`) et **ferme le fichier avant `onFinished`** ; en cas d'annulation ou d'erreur, il ferme sans rien supprimer. | ✔ |
| `IJobObserver` | `onStageChanged(JobStage)` (`Opening`, `PreparingModels`, `Processing`, `Finalizing`), **`onStateChanged(Running / Paused)`**, `onProgress(Progress)`, `onFinished(Expected<JobResult, PipelineError>)` exactement une fois, en dernier. **Libellés UX (Lecture · Agrandissement · Fluidité · Enregistrement) déduits dans le cœur** (fonction pure) de `JobStage` + `OutputPlan`, car les étages tournent en parallèle. | ✎ |
| Fréquence de progression | **4 fois/s (250 ms)**, sous le plafond de 10 Hz ; étapes, états et fin émis immédiatement. `Progress` = images de sortie faites / totales (estimation), i/s de la dernière période, temps écoulé (pauses exclues), VRAM du job, i/s par étage (journal, QA). **Pas d'ETA dans le pipeline** : `EtaEstimator` du cœur (ADR-004). | ✎ |
| Threads des rappels | **Un seul** thread du pipeline (rapporteur), rappels sérialisés, `noexcept`, < 1 ms, **aucun appel en retour vers le pipeline** (ni `pause`/`cancel`, ni `wait`, ni destruction du `JobHandle` : assertion en debug). L'observateur doit survivre au `JobHandle`. L'application reposte vers le thread UI (ADR-002). | ✔ |
| `PipelineError` | `PipelineErrorCode` **calqué sur P-08** (une ligne = un code) : `SourceCorrupt{at}`, `UnsupportedFormat`, `OutOfVram{required_mib, available_mib}`, `DiskFull`, `DeviceLost`, `WriteDenied`, `SourceMissing`, `Internal` ; **hors P-08** : `Cancelled` (pas un échec), `SourceUnreadable` (fichier verrouillé ou droits : **ligne à ajouter à P-08**), `ComponentMissing{component}` (bandeau des prérequis), `OutputExists` (jamais affiché : nom suivant), `InvalidParams` et `Busy` (défauts internes, affichés comme `Internal`). + `detail` technique (anglais) + `native_code`. Aucune exception ne sort de l'API. | ✎ |
| Chemins longs / Unicode | **Tranché** : aucun chemin texte n'est donné à FFmpeg ; E/S par `AVIOContext` maison sur `CreateFileW` avec préfixe `\\?\` (UNC : `\\?\UNC\`) ; ouvertures secondaires de FFmpeg refusées (aucun accès réseau ni fichier hors du job). Test commun sur chemins > 260 caractères, accents, UNC (risque A6). | ✎ |
| Journal | Façade `enh::log` (ADR-007), `av_log` redirigé une seule fois. | ✔ |
| Type d'erreur | **Tranché** : même alias `enh::Expected` (ADR-003, `tl::expected`, CC0) ; le `Result<T>` de la proposition pipeline v0.1 est abandonné. | ✔ |

---

## 13. Pipeline vidéo

Rédigé par le gpu-engineer ; détail, schémas, budgets et code de l'interface dans **`docs/gpu/architecture-pipeline.md`** (v0.2). Les chiffres marqués **[H]** sont à mesurer (ENH-002, ENH-008 à ENH-014) ; aucun en-tête de SDK NVIDIA n'a encore été lu, les noms d'API concernés y sont marqués « à vérifier ».

Résumé :
- **Chaîne** : démux libavformat (CPU) → décodage NVDEC par hwaccel FFmpeg, repli logiciel → conversion couleur, rotation, désentrelacement (noyaux CUDA) → [interpolation RIFE/TensorRT vers 60 i/s si la source est < 50 i/s] → super-résolution RTX Video SDK vers 3840×2160 → encodage NVENC HEVC/AV1 8/10 bits → mux MP4 (bascule MKV), audio et sous-titres copiés sans réencodage.
- **Tout reste sur le GPU** de NVDEC à NVENC : seuls les paquets compressés traversent le bus ; mémoire épinglée uniquement pour le repli logiciel et l'aperçu.
- **Concurrence** : un thread par étage (démux, décodage, traitement, encodage, mux) + un thread rapporteur ; un flux CUDA par étage, passage des images avec un événement CUDA (attente côté GPU) ; files et pools de surfaces **bornés** alloués au démarrage (aucune allocation, aucune synchronisation CPU dans la boucle).
- **VRAM** : budget de job ≤ 9 216 Mio, estimé à ≈ 7 656 Mio **[H]** pour 720p30 → 4K60 ; contrôle avant démarrage et erreur `OutOfVram` propre, jamais de plantage ; pause = VRAM conservée au MVP.
- **Vitesse visée [H]** (720p30 → 4K60, RTX 5070) : Qualité max ≥ 30 i/s, Équilibré ≥ 60 i/s, Rapide ≥ 100 i/s ; facteurs limitants attendus : super-résolution (SM) et NVENC.
- **Interface avec l'application** (§ 12, ADR-051) : `IMediaProbe` et `plan()` sans GPU, `IPipeline::start` → `JobHandle` RAII, observateur à un seul thread, progression 4 fois/s, `enh::Expected<T, PipelineError>` avec codes calqués sur P-08 ; faux pipeline pour la CI.
- **Robustesse** : DLL NVIDIA chargées dynamiquement (l'application démarre sans carte compatible), E/S Win32 maison (chemins longs, aucune ouverture secondaire ni réseau), toute ressource C encapsulée en RAII, aucune exception aux frontières.

ADR du pipeline (plage ADR-040 et plus) — licences à valider par secdevops dans `docs/security/licences.md` :

| ADR | Décision | Écarté | Licence de ce qui est retenu |
|---|---|---|---|
| ADR-040 | Décodage NVDEC via **hwaccel FFmpeg** (`AV_HWDEVICE_TYPE_CUDA`), repli logiciel par la même API | NVDEC direct (Video Codec SDK), décodeurs `*_cuvid` | FFmpeg **LGPL-2.1+** liaison dynamique (build `lgpl-shared`) ; `nv-codec-headers` MIT ; `nvcuvid.dll` fournie par le pilote |
| ADR-041 | Encodage **NVENC natif** (`nvEncodeAPI.h`), entrée CUDA, mode asynchrone | `hevc_nvenc`/`av1_nvenc` de libavcodec ; `libx265` (GPL, interdit), SVT-AV1 (trop lent) | En-tête Video Codec SDK (MIT, à vérifier) ; `nvEncodeAPI64.dll` fournie par le pilote |
| ADR-042 | Couleur : **noyaux CUDA maison**, pivot RGB FP16, matrice / primaires / transfert / plage explicites, formules testées sur CPU en CI | `scale_cuda`/`zscale`, NPP | Notre code ; CUDA Toolkit (EULA NVIDIA, `cudart` statique) |
| ADR-043 | Interpolation **RIFE 4.x + TensorRT** (cible), **FRUC Optical Flow** en repli ; moteur construit localement et mis en cache | RIFE via ONNX Runtime, `minterpolate` | Code RIFE MIT ; **poids : licence à confirmer (bloquant)** ; TensorRT et FRUC : licences NVIDIA, redistribution à vérifier |
| ADR-044 | **Interpolation avant la super-résolution** (provisoire, tranché par mesure dans ENH-002) | Interpolation à 4K (hors budget VRAM dans le pire cas) | — |
| ADR-045 | **Un thread par étage + flux CUDA + événements**, files bornées | Boucle mono-thread, pool de tâches + graphes CUDA | — |
| ADR-046 | **Pause avec VRAM conservée** (MVP) ; libération de la carte en pause plus tard, avec la reprise après plantage | Pause qui détruit et reconstruit le job | — (**à valider par le PO**) |
| ADR-047 | Erreurs : **`enh::Expected<T, PipelineError>`** (alias de l'ADR-003), `check()` sur chaque code retour C, aucune exception | Type `Result` propre au pipeline | tl::expected **CC0-1.0** |
| ADR-048 | **DLL NVIDIA chargées dynamiquement** (System32 pour le pilote, dossier de l'application pour les SDK), `cudart` statique ; amendement 1 (ENH-040) : pont RTX Video chargé explicitement par chemin absolu, processus enfants interdits pendant l'init NGX, DLL de fonction vérifiée par son contenu (`docs/gpu/architecture-pipeline.md`) | Liaison implicite | — |
| ADR-049 | Fichiers : **l'application possède le cycle de vie** ; le pipeline crée le chemin reçu en `CREATE_NEW` ; **E/S FFmpeg par `AVIOContext` Win32** (`\\?\`), ouvertures secondaires refusées | `.enhance-part` géré par le pipeline ; chemins UTF-8 / 8.3 donnés à FFmpeg | Notre code (API Win32) |
| ADR-050 | Conteneur **MP4, bascule MKV** décidée par `plan()` avant traitement ; SRT/WebVTT convertis en `mov_text` | — | FFmpeg LGPL (déjà listé) |
| ADR-051 | **Contrat pipeline ↔ application** : `IMediaProbe`, `plan()` pur, `JobHandle` RAII, observateur à un seul thread, progression 4 fois/s, codes P-08 | `unique_ptr<IJob>` + `shared_ptr` d'observateur ; ETA dans le pipeline ; rappels autorisés à rappeler le pipeline | Notre code |
| ADR-052 | **Pilote NVIDIA minimal 610** (pilote de référence des mesures : 617.42) ; Video Codec SDK 13.1 | Pilote plus ancien non supporté ; détail et raisons dans `docs/gpu/architecture-pipeline.md` § ADR-052 | — |

---

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

## 15. Build, dépendances, CI, packaging (devops)

Statut : proposition devops du 2026-10-21 (ADR-020 à ADR-037), **à valider par le PO avant toute installation**. Rien n'est encore installé ni téléchargé. **[H]** = hypothèse à vérifier à l'étape 5 (installation) ou en ENH-003.

Principe directeur : **deux mondes de build**.
- **Monde sans GPU** (CI, et tout poste sans SDK) : `ENH_WITH_GPU=OFF`, pipeline simulé (`FakePipeline`), aucune DLL NVIDIA, aucun SDK. C'est ce que la CI compile.
- **Monde GPU** (machine du PO) : `ENH_WITH_GPU=ON`, CUDA Toolkit + SDK dans `third_party/nvidia/`, tests `tests/gpu/`.

### 15.1 ADR-020 — Toolchain : Visual Studio 2026 (MSVC v145), CMake, Ninja, presets

**Contexte.** Décision PO : VS 2026 (18.9, MSVC 14.51, toolset v145) au lieu de VS 2022 ; c'est le seul VS de la machine. Le PO apprend le C++ : le build doit tenir en une commande. Le `cmake` du PATH est celui de MSYS2 MinGW (à éviter) ; VS embarque CMake 4.3.1 + Ninja.

**Décision.**
- Compilateur : **MSVC v145 (`cl` 19.5x), x64 uniquement**. Pas de clang-cl ni de MinGW pour le produit.
- CMake : `cmake_minimum_required(VERSION 3.30)` (support CUDA 13, `CMAKE_CUDA_ARCHITECTURES`, `FILE_SET`, presets v6) ; version de référence = celle de VS (4.3.1). Les scripts localisent **ce** CMake et **ce** Ninja via `vswhere` (chemin absolu) : aucune modification du PATH de la machine, MSYS2 est ignoré.
- Générateur : **Ninja** (rapide, `compile_commands.json` gratuit pour clang-tidy, seul à bien gérer CUDA + MSVC en une passe), dans l'environnement « x64 Native Tools » chargé par `scripts/build.ps1` (`Launch-VsDevShell.ps1 -Arch amd64`). Dans l'IDE VS, les presets fonctionnent aussi (ils déclarent `architecture`/`toolset` en stratégie `external`).
- `CMakePresets.json` (versionné) : presets cachés `base`, `gpu`, `nogpu` ; presets visibles `dev` (RelWithDebInfo, GPU auto), `dev-nogpu`, `release`, `ci` (nogpu, `/WX`). Les chemins locaux (SDK, VCPKG_ROOT) vont dans `CMakeUserPresets.json` (ignoré par git), jamais dans les presets versionnés.
- **Pas de configuration Debug** pour l'application Qt (voir ADR-024 : Qt n'est compilé qu'en release) ; `RelWithDebInfo` (`/MD`, symboles PDB, `/Od` possible par cible) sert au débogage. Les tests du seul cœur (sans Qt) peuvent aussi tourner en Debug + ASan (ADR-030).
- Options CMake : `ENH_WITH_GPU` (`AUTO`|`ON`|`OFF`), `ENH_WITH_UI` (`ON`|`OFF`), `ENH_BUILD_TESTS`, `ENH_NVIDIA_SDK_DIR`, `ENH_FFMPEG_ROOT`. `AUTO` = GPU si tout est trouvé, sinon message clair listant l'élément manquant et où le poser, puis bascule sans GPU.

**Alternatives écartées.**
1. *Générateur « Visual Studio 18 2026 » (MSBuild)* : plus lent, pas de `compile_commands.json`, CUDA via « build customizations » à installer ; reste utilisable à la main mais non supporté par les scripts.
2. *Installer VS 2022 en parallèle* : contraire à la décision du PO, double maintenance ; réservé au repli v143 de l'ADR-021.

**Conséquences.** Un seul chemin de build documenté (`scripts/build.ps1 -Preset dev`). Le premier `configure` échoue proprement si VS < 18.0 ou si le toolset n'est pas v145.
**Licence.** Outils de build (CMake BSD-3-Clause, Ninja Apache-2.0, MSVC licence VS Community) : non redistribués. Rappel : VS Community convient pour un projet personnel/OSS ; à revérifier si le projet devenait commercial (secdevops).

### 15.2 ADR-021 — CUDA Toolkit 13.x et compilateur hôte

**Contexte.** RTX 50 : `sm_120`, CUDA ≥ 12.8 ; pilote 617.42 → runtime ≤ 13.4. `nvcc` refuse un compilateur hôte MSVC qu'il ne connaît pas (« unsupported Microsoft Visual Studio version »). Seuls les fichiers `.cu` (`src/pipeline/cuda/`) passent par `nvcc` ; tout le reste est compilé par `cl` directement.

**Décision.**
- **CUDA Toolkit 13.x, version exacte épinglée** dans `docs/architecture.md` après installation : la plus récente ≤ 13.4 dont les notes de version listent **VS 2026 / MSVC 14.5x** comme hôte pris en charge **[H]** (à vérifier sur la page des notes de version avant le téléchargement).
- Cibles : `CMAKE_CUDA_ARCHITECTURES=120-real` **plus** PTX `120-virtual` (compatibilité avant) ; pas de `sm_50…` hérités. `find_package(CUDAToolkit)` ; liaison de `cudart` **statique** (aucune DLL CUDA runtime à redistribuer) ; NVRTC/cuBLAS non utilisés tant que le gpu-engineer ne l'exige pas.
- **Repli v143** si `nvcc` refuse v145 : ajouter à VS 2026 le composant « MSVC v143 – Outils de build VS 2022 (x64) » et ne passer que `CMAKE_CUDA_HOST_COMPILER` (chemin du `cl.exe` v143) pour la bibliothèque `.cu` ; le reste du code reste en v145 (ABI C++ stable entre v143 et v145 pour les bibliothèques statiques ; l'interface entre les deux est C ou RAII pur, sans type STL exposé). `-allow-unsupported-compiler` : dépannage local uniquement, jamais dans les presets versionnés ni la CI.
- Absence de CUDA en monde sans GPU : aucun impact (`project(... LANGUAGES CXX)` ; `CUDA` activé seulement si `ENH_WITH_GPU` effectif).

**Alternatives écartées.**
1. *Rester en CUDA 12.8/12.9* : listent seulement VS 2022 ; le pilote 617.42 accepte 13.x ; pas de gain.
2. *Compiler les noyaux en NVRTC à l'exécution* : supprime `nvcc` du build mais ajoute compilation au lancement, DLL NVRTC à redistribuer (licence à vérifier) et débogage plus difficile.

**Conséquences.** Le CUDA Toolkit n'est **pas** redistribué (liaison statique de `cudart`) : pas d'obligation de licence côté livrable. `test-gpu.ps1` vérifie que `nvcc --version` correspond à la version épinglée.
**Licence.** CUDA Toolkit EULA NVIDIA : usage de développement ; éléments redistribuables listés dans l'« Attachment A » (secdevops à valider si une DLL CUDA devait être livrée).

### 15.3 ADR-022 — Dialecte C++20, avertissements, exceptions

**Contexte.** Stack imposée : C++20 « sans exception ». Qt, la STL, FFmpeg (en-têtes) et nlohmann/json supposent que les exceptions existent.

**Décision.**
- `CMAKE_CXX_STANDARD 20`, `CXX_EXTENSIONS OFF` ; MSVC : `/std:c++20 /permissive- /Zc:__cplusplus /utf-8 /W4 /WX` (la CI et `release` ; `dev` garde `/W4` sans `/WX` pour ne pas freiner l'apprentissage), `/EHsc`, `/Zc:preprocessor`, `/MD` partout, `/guard:cf` et `/GS` en release, ASLR/DEP/CFG par défaut de l'éditeur de liens (`/DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA`).
- **« Sans exception » = politique, pas drapeau** : `/EHsc` reste activé car Qt, la STL et les tiers peuvent en lancer ; aucune exception ne traverse une API d'Enhance (ADR-003) ; `throw` interdit dans `src/` (règle de revue + clang-tidy `hicpp-exception-baseclass` / `bugprone-exception-escape`). Pas de `/EHs-c-` (provoquerait des comportements indéfinis avec la STL).
- `tl::expected` (ADR-003) via vcpkg, alias `enh::Expected` défini dans `src/common/include/enh/expected.hpp` (cible `enh_common`, ADR-037).
- Warnings des en-têtes tiers en `SYSTEM` (Qt, FFmpeg, SDK NVIDIA) pour que `/WX` ne casse pas sur du code qui n'est pas le nôtre.

**Alternatives écartées.**
1. *`/EHs-c-` (exceptions réellement désactivées)* : incompatible avec Qt/STL, gain de taille négligeable.
2. *C++23 (`std::expected`)* : hors du cadre imposé (ADR-003) ; migration prévue en une ligne le moment venu.

**Conséquences.** `static_assert`/concepts disponibles ; les modules C++20 ne sont pas utilisés (support CMake/Ninja/moc encore fragile).
**Licence.** Aucune brique nouvelle.

### 15.4 ADR-023 — Dépendances : vcpkg en mode manifeste, baseline épinglée

**Contexte.** Dépendances C++ à versions reproductibles, sans compte, sans binaire commité. vcpkg est absent de la machine.

**Décision.**
- **vcpkg en mode manifeste** : `vcpkg.json` (versionné) + `vcpkg-configuration.json` avec `"default-registry": { "kind": "git", "baseline": "<sha du commit vcpkg>" }` **épinglé** ; l'outil vcpkg lui-même cloné au **même commit** (`git -C C:\Tools\vcpkg checkout <sha>`), noté dans le README. Remontée de baseline = une PR dédiée, avec rebuild complet et test.
- Triplet maison `cmake/triplets/x64-windows-enh.cmake` (versionné) : `VCPKG_TARGET_ARCHITECTURE x64`, `VCPKG_CRT_LINKAGE dynamic`, `VCPKG_LIBRARY_LINKAGE static` (spdlog, gtest… dans l'exécutable), `VCPKG_BUILD_TYPE release` (voir ADR-024), Qt forcé en dynamique par port (`if(PORT MATCHES "^qt")  set(VCPKG_LIBRARY_LINKAGE dynamic)`).
- Contenu du manifeste (versions fixées par la baseline) :
  - toujours : `gtest`, `spdlog` (feature `std-format` pour éviter fmt, à confirmer), `nlohmann-json`, `tl-expected` ;
  - feature `ui` (défaut local, **désactivée en CI**, voir ADR-029) : `qtbase`, `qtdeclarative`, `qtsvg`, `qtshadertools`, `qttools` (outils de traduction), avec options de l'ADR-024 ;
  - feature `ffmpeg` (option, ADR-025) : `ffmpeg` LGPL.
- Cache binaire : `VCPKG_DEFAULT_BINARY_CACHE=F:\vcpkg-cache` (local au PO, hors dépôt) ; `buildtrees` déportés dans un chemin court (`--x-buildtrees-root=C:\vb`) pour éviter les chemins > 260 caractères pendant la compilation de Qt.
- Télémétrie vcpkg désactivée (`VCPKG_DISABLE_METRICS=1`, `-disableMetrics`).
- Le **réseau est utilisé au build seulement** (téléchargement des sources par vcpkg) ; l'application, elle, n'en a jamais besoin (ADR-027).

**Alternatives écartées.**
1. *Conan 2* : bon outil mais Python à installer, recettes Qt moins éprouvées sous MSVC, une langue de plus pour le PO.
2. *CMake FetchContent* : bien pour 3-4 en-têtes, mais recompile Qt/FFmpeg à chaque configure propre, pas de cache binaire partagé, empreintes à gérer à la main (on l'accepte seulement pour un éventuel en-tête isolé, avec `URL_HASH`).
3. *Sous-modules git* : histoire du dépôt alourdie, mises à jour manuelles.

**Conséquences.** Premier configure long (Qt, voir ADR-024), les suivants instantanés grâce au cache. `docs/security/licences.md` se déduit des ports vcpkg (`vcpkg_installed/.../share/*/copyright`) : un script `scripts/licences.ps1` les rassemble dans `THIRD-PARTY-LICENSES.txt` (ADR-032).
**Licence.** vcpkg : MIT (outil) ; chaque port garde la licence de sa bibliothèque : gtest BSD-3, spdlog MIT, nlohmann-json MIT, tl-expected CC0, Qt LGPLv3.

### 15.5 ADR-024 — Qt 6 sans réseau : compilé via vcpkg, sans `Qt6Network`

**Contexte.** Risque du developer : dans les binaires officiels, `Qt6Qml.dll` importe `Qt6Network.dll` (fonction `qml-network`), contraire à l'exigence « zéro réseau » (ENH-028, preuve par analyse des dépendances). Qt 6.8 LTS minimum ; vcpkg fournit la version de la baseline.

**Décision.**
- **Qt compilé par vcpkg depuis les sources, sans le module Network** : `qtbase` avec `default-features: false` et uniquement les fonctions utiles (`gui`, `harfbuzz`, `freetype`, `png`, `jpeg`… ; `network`, `sql`, `widgets` (ou widgets seulement si exigé par un outil), `dbus`, `opengl`/`vulkan` selon besoin RHI D3D11 uniquement) ; `qtdeclarative` avec `qml-network` désactivé. Si vcpkg n'expose pas ces options **[H]**, on ajoute un **overlay port** versionné (`vcpkg/overlay-ports/qtbase`, `qtdeclarative`) qui passe `-DFEATURE_network=OFF` puis `-DFEATURE_qml_network=OFF` ; la configuration de build (triplet + overlay) est dans le dépôt, ce qui satisfait l'obligation LGPL de fournir la configuration.
- RHI : **Direct3D 11 seulement** (pas de Vulkan, pas d'OpenGL) ; styles Quick Controls : Basic uniquement (les styles Windows/Fluent ne sont pas compilés/déployés).
- Compilation **en release uniquement** (`VCPKG_BUILD_TYPE=release`) : divise le temps par ~2 et l'espace disque par ~2 ; conséquence : pas de config Debug de l'application (ADR-020).
- **Temps de compilation estimé [H]** (machine du PO, 8 à 16 threads logiques, SSD) : qtbase 20–30 min, qtshadertools 3–5, qtdeclarative 20–30, qtsvg 3, qttools 8–12 → **≈ 1 h à 1 h 20 une seule fois**, puis quelques secondes (cache binaire). Avec Debug en plus : ≈ 2 h à 2 h 30. Espace temporaire : 15–25 Gio dans `C:\vb` (supprimable ensuite), installé ≈ 1,5–2,5 Gio, cache ≈ 1 Gio. Une remontée de baseline qui change Qt refait ce build.
- Vérification : `scripts/check-no-network.ps1` (ADR-027) échoue si `Qt6Network.dll` existe dans le dossier déployé ou est importée.

**Alternatives écartées.**
1. *Binaires officiels (installeur Qt, `aqt`)* : rapides (5 min), mais `Qt6Network.dll` obligatoire et compte Qt requis pour l'installeur ; gardés seulement comme **plan B de CI** (ADR-029, tests uniquement, jamais livrés) et plan B de livraison ci-dessous.
2. *Qt compilé à la main hors vcpkg (`configure -no-feature-network`)* : plus de contrôle, mais pas de cache binaire ni de baseline reproductible ; plan C si vcpkg bloque.

**Plan B (si le Qt sans réseau échoue ou si le build est impraticable).** Livrer les binaires officiels **avec** `Qt6Network.dll` (impossible de la retirer : import statique de `Qt6Qml`) et prouver l'absence de trafic autrement : aucune création de socket (point d'arrêt/Detours en test, `netstat`/Wireshark pendant un traitement, règle de pare-feu sortante « Bloquer » appliquée à `enhance.exe` et test de bon fonctionnement), liste blanche documentée avec secdevops. Décision à valider par le PO et secdevops uniquement si le plan A est impossible.

**Conséquences.** Le premier lancement de l'étape 5 compile Qt pendant ≈ 1 h 20 : à lancer pendant que le PO fait autre chose. Les sources de Qt utilisées (archives téléchargées par vcpkg dans `downloads/`) sont conservées pour l'obligation LGPL.
**Licence.** Qt 6 : LGPLv3 (modules listés à l'ADR-001) ; outils de build (`moc`, `rcc`, `qsb`, `lupdate`) non redistribués. Obligations : DLL séparées, textes de licence et mention « À propos », sources/configuration disponibles.

**Mise en œuvre (étape 5, phase B, 2026-10-21) — plan A obtenu : Qt 6.11.2 sans `Qt6Network`.**
- Baseline vcpkg `a2f9862da6d88040a8537fb9b8cb24deaa9a196b` (clone `--filter=blob:none` dans `C:\Tools\vcpkg`, outil vcpkg 2026-09-26) ; Qt **6.11.2**.
- Le port `qtbase` expose bien la feature `network` (désactivée via `default-features: false`), mais `qtdeclarative`, `qtlanguageserver` et `qttools[linguist→designer]` la **réclament** : trois **overlay ports** versionnés dans `vcpkg/overlay-ports/` (déclarés par `vcpkg-configuration.json`) retirent cette dépendance ; `qtdeclarative` passe en plus `-DFEATURE_qml_network=OFF`. `qttools` ne fournit plus que `linguist` (lupdate/lrelease), sans `designer`/`assistant`.
- Résultat vérifié : aucun `Qt6Network*.dll`, aucun plugin `tls`/`networkinformation` dans `vcpkg_installed` ; `Qt6Core.dll` importe encore `ws2_32` (`WSAAsyncSelect`, `WSAStartup`, `WSACleanup` : boucle d'événements Windows), point soumis à secdevops (exception E2 de `scripts/network-allowlist.json`).
- Durée réelle du build complet (8 ports Qt + dépendances, configure inclus) : **35 min**, 25 Gio temporaires dans `C:\vb`. Spdlog : feature `fmt` retirée (`default-features: false`) pour utiliser `std::format`.

### 15.6 ADR-025 — FFmpeg : LGPL, liaison dynamique

**Contexte.** Décision PO n° 4 : FFmpeg LGPL en DLL uniquement (pas de GPL, pas de `--enable-nonfree`). NVDEC passe par le hwaccel FFmpeg (ADR-P01), NVENC en direct (ADR-P02) : FFmpeg sert au démux/mux, au décodage logiciel de repli et à `swscale` éventuel.

**Décision.**
- **Phase spike/développement : build BtbN `win64-lgpl-shared`** déposé dans `third_party/ffmpeg/lgpl-shared/` (branche 8.x), **épinglé par tag dédié `autobuild-AAAA-MM-JJ-HH-MM` ou `n8.x.y` + somme SHA-256** consignés dans `deps/ffmpeg.lock.json` (versionné, hors `third_party/` : SEC-17 ; sans le binaire ; épinglé le 2026-10-21 : tag `autobuild-2026-10-20-13-08`, FFmpeg n8.1.3-18-g46d899d7a0) ; `scripts/fetch-ffmpeg.ps1` télécharge, **vérifie le SHA-256**, extrait ; échec si la somme diffère. Le script ne s'exécute qu'à la demande (jamais en CI).
- **Avant la première release : bascule vers le port vcpkg `ffmpeg`** (feature LGPL minimale : `avcodec avformat avutil swscale swresample`, sans `gpl`/`nonfree`, avec `nvcodec`), car les tags BtbN datés sont purgés au bout de quelques semaines (non reproductible) et nous devons fournir les sources correspondant aux DLL livrées. CMake ne voit qu'une cible `Enhance::ffmpeg` (`ENH_FFMPEG_ROOT` ou `find_package(FFmpeg)`) : la bascule ne touche pas le code.
- Contrôles automatiques (`scripts/check-ffmpeg-license.ps1`) : `ffmpeg -version` / `avcodec_configuration()` ne doit contenir ni `--enable-gpl` ni `--enable-nonfree` ni `libx264`/`libx265`/`libfdk` ; protocoles réseau à désactiver à l'exécution (`protocol_whitelist=file`, côté gpu-engineer) et bibliothèques réseau signalées à secdevops (ADR-027).
- Un FFmpeg **« outil » GPL avec libvmaf** (QA) vit hors dépôt (`C:\Tools\ffmpeg`), n'est jamais lié ni livré.

**Alternatives écartées.**
1. *vcpkg `ffmpeg` dès le début* : meilleur à terme (reproductible, sources), mais +15–30 min de compilation et le temps de valider NVDEC dans cette configuration, alors que le spike doit avancer vite.
2. *Build maison MSYS2* : contrôle total, maintenance lourde, risque d'activer du GPL par erreur.

**Conséquences.** Obligations LGPL : DLL FFmpeg séparées, texte LGPLv2.1+, mention, lien vers les sources et le mode de configuration, aucune interdiction de rétro-ingénierie dans la licence d'Enhance.
**Licence.** FFmpeg LGPLv2.1+ (build sans GPL) ; secdevops valide le build exact avant redistribution.

### 15.7 ADR-026 — SDK NVIDIA : `third_party/nvidia/`, détectés, jamais commités

**Contexte.** RTX Video SDK, Video Codec SDK, Optical Flow SDK, TensorRT : licences NVIDIA, compte requis, redistribution à valider (R6). Le PO les dépose lui-même ; le gpu-engineer les charge dynamiquement (ADR-P09), donc **seuls les en-têtes** sont nécessaires à la compilation, sauf TensorRT (bibliothèque d'import).

**Décision.**
- Racine : `third_party/nvidia/` (ignorée par git) ou toute autre via variable d'environnement `ENH_NVIDIA_SDK_DIR` / option `-DENH_NVIDIA_SDK_DIR=` (priorité : option > environnement > défaut). Sous-dossiers attendus : `rtx-video-sdk\`, `video-codec-sdk\`, `optical-flow-sdk\`, `tensorrt\`.
- `cmake/EnhNvidia.cmake` : `enh_find_nvidia_sdk(<nom> <fichier-sentinelle>)` cherche un fichier précis (ex. `Interface\nvEncodeAPI.h`, `include\NvInfer.h`) et crée une cible `INTERFACE IMPORTED` `NVIDIA::<nom>`. **Message d'erreur fixe** : « SDK NVIDIA manquant : <nom>. Attendu : <chemin>\<sentinelle>. Voir docs/gpu/spike.md §2.1. Pour compiler sans GPU : -DENH_WITH_GPU=OFF ». `ENH_WITH_GPU=ON` + manque → `FATAL_ERROR` ; `AUTO` → bascule sans GPU avec ce même message en `WARNING`.
- Les versions des SDK sont lues dans leurs fichiers de version et affichées à la fin du configure ; `docs/gpu/spike.md` §2.1 (tableau « Version réellement téléchargée ») et `docs/security/licences.md` restent les registres.
- Aucune DLL NVIDIA n'est copiée dans le livrable tant que secdevops n'a pas marqué sa redistribution « oui » ; en attendant, le pilote fournit NVDEC/NVENC/NVML et le portable documente les composants à installer.
- Garde-fou git : un test de `scripts/precommit-check.ps1` refuse tout fichier de `third_party/` ou toute extension `.dll .lib .exe .zip .onnx .engine` hors liste blanche.

**Alternatives écartées.**
1. *Commiter les SDK (Git LFS)* : contraire à la règle du PO et incompatible avec la licence NVIDIA (redistribution des SDK).
2. *Chemins codés en dur dans les CMakeLists* : casse chez chaque contributeur, risque d'exposer un chemin personnel dans le dépôt.

**Conséquences.** Tout poste sans SDK (CI comprise) compile et teste le monde sans GPU.
**Licence.** NVIDIA SDK EULA/licences propres : consultation et compilation seulement ; redistribution des DLL = décision secdevops par composant.

### 15.8 ADR-027 — Preuve « zéro réseau » côté build

**Contexte.** ENH-028 exige une méthode de vérification ; secdevops en est propriétaire, devops fournit les contrôles automatisables.

**Décision.** `scripts/check-no-network.ps1` (local + job CI) sur le dossier déployé : (1) interdit `Qt6Network.dll`, `QtNetwork*`, `winhttp.dll`, `wininet.dll`, `libcurl*`, `cpprest*` ; (2) analyse `dumpbin /dependents` (et `/imports` ciblé) de chaque exécutable et DLL : tout import de `ws2_32.dll`/`mswsock.dll`/`winhttp.dll` doit figurer dans une **liste blanche commentée** (`scripts/network-allowlist.json`, validée par secdevops : ex. `ws2_32` tiré par `avformat` même sans usage) ; (3) en local, test dynamique à la demande : règle de pare-feu sortante « Bloquer » sur `enhance.exe` puis exécution d'un traitement complet (doit réussir). Le `libavformat` livré est appelé avec la liste blanche de protocoles `file` uniquement (gpu-engineer).

**Alternatives écartées.**
1. *Seulement des revues manuelles* : non reproductible.
2. *Bac à sable réseau (Windows Sandbox sans réseau)* : bon complément manuel, trop lourd pour être systématique.

**Conséquences.** La liste blanche est un livrable de sécurité (modification = revue secdevops). **Licence.** Outils Microsoft (`dumpbin`) non redistribués.

### 15.9 ADR-028 — Rendu QML en CI sans GPU

**Contexte.** Les runners n'ont pas de carte ; les tests Qt Quick Test doivent valider liaisons, états, clavier, accessibilité (ADR de tests § 14).

**Décision.** `QT_QPA_PLATFORM=offscreen` + `QT_QUICK_BACKEND=software` (rendu logiciel de Qt Quick) pour tous les tests QML/Qt Test en CI et en local par défaut. Limite connue : le backend logiciel n'exécute pas les shaders de `MultiEffect` (flou) ; ces effets sont testés par état/propriétés, et leur rendu l'est **en local** (D3D11 réel, captures `grabWindow`). Option avancée, non bloquante : job manuel avec la plateforme `windows` + `QSG_RHI_BACKEND=d3d11` + `QSG_RHI_PREFER_SOFTWARE_RENDERER=1` (WARP) pour comparer des captures. Chaque test fixe son scale (`QT_SCALE_FACTOR` 1 et 2) et ses polices (police embarquée, pas la police système).

**Alternatives écartées.** 1. *Runner GPU auto-hébergé* : exécuterait du code de PR sur la machine du PO (refusé sauf demande explicite + avis secdevops). 2. *Ne pas tester le QML en CI* : perd le filet de sécurité de l'interface.

**Conséquences.** Les captures de référence sont comparées en local seulement. **Licence.** Qt Quick Test (LGPL/GPL) : tests uniquement.

### 15.10 ADR-029 — CI GitHub Actions (`rolkapone/P153`, privé)

**Contexte.** Dépôt privé : minutes Actions limitées (offre gratuite : 2000 min/mois, **Windows facturé ×2** = ≈ 1000 min Windows ; runners privés 2 vCPU **[H]**). Pas de GPU. Un build complet de Qt dans le cloud (≈ 3–4 h sur 2 vCPU) est trop coûteux.

**Décision.** Fichier `.github/workflows/ci.yml`, déclencheurs `push` sur `main` et `pull_request` ; `paths-ignore: docs/**, *.md` ; `concurrency` avec annulation des exécutions obsolètes ; `permissions: contents: read` ; **toutes les actions épinglées par SHA de commit** (avec la version en commentaire) ; runner **épinglé** (`windows-2025`, ou l'image VS 2026 si disponible **[H]** — sinon toolset v143 en CI, écart documenté) ; aucun secret ; **aucun push, aucune publication**.
Jobs :
1. `format` (ubuntu, ×1, ≈ 1 min) : `clang-format --dry-run --Werror` sur les fichiers modifiés (version épinglée identique au local, ADR-030).
2. `core` (windows) : configure `ci` (`ENH_WITH_GPU=OFF`, `ENH_WITH_UI=OFF`), build, CTest label `unit` (GoogleTest, FakePipeline) ; variante ASan. vcpkg avec cache binaire (`actions/cache` sur le dossier du cache) ; ≈ 5–8 min.
3. `tidy` (windows) : `clang-tidy` sur les fichiers de `src/` présents dans `compile_commands.json` du preset `ci` (sans GPU ni UI) ; le reste (FFmpeg, CUDA, Qt) est couvert en local par `scripts/tidy.ps1 -Preset local-gpu` avant merge des stories [GPU]/[UI] (ADR-030).
4. `ui` (windows) : Qt Quick Test + Qt Test, avec **Qt officiel installé par `install-qt-action` (version identique à celle de vcpkg, MSVC 64 bits)**, **uniquement pour exécuter les tests** (le Qt livré est celui de l'ADR-024) ; `QT_QPA_PLATFORM=offscreen`, ADR-028. Exécuté sur pull request vers `main` et manuellement, pas à chaque push de branche, pour économiser les minutes.
5. `security` (emplacement pour secdevops, à définir avec lui) : `check-no-network.ps1` sur un build `ci`, inventaire des licences (`scripts/licences.ps1`) comparé à `docs/security/licences.md`, scan de secrets (gitleaks, binaire épinglé), audit des dépendances, interdiction de fichiers binaires/SDK. CodeQL C++ pour un dépôt privé exige GitHub Advanced Security (payant) : non retenu par défaut.
6. **Aucun job GPU, aucun runner auto-hébergé.**
Coût visé : ≈ 15–25 min Windows facturées ×2 par exécution complète → surveiller dans Réglages > Billing, alerte à 75 %, `timeout-minutes` sur chaque job.

**Alternatives écartées.** 1. *Tout en CI avec Qt vcpkg* : ≈ 4 h de premier build ou cache de plusieurs Gio à semer, sans garantie de réutilisation entre branches. 2. *Pas de CI, tout en local* : contraire à « `main` toujours vert » vérifié indépendamment du poste du PO.

**Conséquences.** Le merge reste conditionné aux tests **locaux** (build + unitaires + `test-gpu.ps1`) ; la CI est le second filet. Si le dépôt devient public, les minutes ne comptent plus.
**Licence.** Actions tierces : vérifier la licence de chaque action choisie (la plupart MIT) ; GitHub-hosted runners : conditions GitHub.

### 15.11 ADR-030 — Qualité : clang-format, clang-tidy, sanitizers, couverture

**Contexte.** Qualité homogène et débutant-friendly ; formats identiques entre le poste et la CI.

**Décision.**
- `.clang-format` (base LLVM, `ColumnLimit: 100`, `IndentWidth: 4`, `SortIncludes`, fins de ligne gérées par `.gitattributes`) ; **version unique épinglée** (**22.1.3**, épinglée à l'étape 5 : c'est celle du composant LLVM de VS 2026 ; `pip install clang-format==22.1.3` en CI) : localement via `winget install LLVM.LLVM --version <x.y.z>` ou composant VS « Outils clang », en CI via `pip install clang-format==<x.y.z>`.
- `.clang-tidy` : `bugprone-*`, `performance-*`, `modernize-*` (sauf `use-trailing-return-type`), `readability-*` (sélection), `cppcoreguidelines-*` (sélection : ownership, `no-malloc`, `owning-memory`), `misc-*` ; `WarningsAsErrors` limité à `bugprone-*` et `cppcoreguidelines-owning-memory` ; exclusion de `build/`, `moc_*`, `qrc_*`, `third_party/`, `*.cu`. Interdit de garder `new`/`delete` nus hors encapsulation RAII.
- **ASan** (`/fsanitize=address`, supporté par MSVC) sur la suite du cœur en CI ; **UBSan non disponible sous MSVC** (noté).
- Couverture : **OpenCppCoverage** (outil local, GPL-3.0, non redistribué, non lié) via `scripts/test.ps1 -Coverage` ; rapport HTML dans `build/coverage/` ; seuil indicatif (cœur ≥ 80 %, machine à états ≥ 90 %), non bloquant en CI au début.
- `scripts/format.ps1` (applique) et `scripts/test.ps1` (CTest label `unit`, + `ui` si Qt disponible).
- **Réglages CI (amendement du 2026-10-24, premier run).** `CMAKE_CXX_SCAN_FOR_MODULES=OFF` dans le preset `base` (Ninja + MSVC générait des commandes `@...modmap` illisibles par clang-tidy ; **à revoir si l'équipe adopte les modules C++20**). `bugprone-easily-swappable-parameters` désactivé (bruit sans valeur sur ce code ; interfaces à paramètres de même type usuels). cppcheck : `--suppress=unknownMacro` (macros Qt `Q_PROPERTY`, `Q_IMPORT_QML_PLUGIN`). Le job `tidy` n'analyse que les fichiers présents dans `build/ci/compile_commands.json` : les fichiers FFmpeg, CUDA et Qt (≈ 10) n'ont pas leurs en-têtes sur le runner.
- **Filet local** : `scripts/tidy.ps1 -Preset local-gpu` (ou `dev`) analyse tous les fichiers de `src/` du preset choisi ; à lancer **avant le merge des stories [GPU] et [UI]**.

**Alternatives écartées.** 1. *`clang-format` ≠ versions entre poste et CI* : diffs sans fin. 2. *PVS-Studio/Sonar* : payants ou compte requis.

**Conséquences.** Premier passage de `clang-tidy` en avertissements, durci au fil des sprints. **Licence.** LLVM Apache-2.0 w/ LLVM exception ; OpenCppCoverage GPL-3.0 (outil seul).

### 15.12 ADR-031 — Tests GPU locaux : `scripts/test-gpu.ps1`

**Contexte.** La CI n'a pas de RTX ; la Definition of Done exige des tests GPU verts et une absence de fuite VRAM.

**Décision.** `scripts/test-gpu.ps1 [-Preset dev] [-Quick] [-Story ENH-0xx]` :
1. **Vérifie** (échec clair, code de sortie ≠ 0) : carte NVIDIA avec capacité de calcul ≥ 12.0 (`nvidia-smi --query-gpu=name,driver_version,compute_cap,memory.total --format=csv`), pilote ≥ 617.42 (valeur lue dans un fichier de configuration), `nvcc` à la version épinglée, SDK présents (`enh_find_nvidia_sdk` en mode vérification), FFmpeg LGPL (`check-ffmpeg-license.ps1`), espace disque et VRAM libre au départ (moins de 1,5 Gio utilisés par d'autres applications, sinon avertissement).
2. **Build** `dev` avec `ENH_WITH_GPU=ON`.
3. **Lance** CTest label `gpu` (`tests/gpu/` : NVDEC, noyaux CUDA, NVENC HEVC/AV1, RTX Video, interpolation, annulation/pause) puis la **boucle VRAM** (10 petits traitements à la suite : VRAM résiduelle ≤ 50 Mio au-dessus du départ) et les **mesures QA rapides** (images/s sur `REF-A` si le clip est présent, sinon « non mesuré » signalé, pas de faux vert). `-Quick` omet les mesures longues.
4. **Écrit** `docs/qa/resultats/AAAA-MM-JJ_HHMM_<branche>.md` (horodaté ; contient la machine, pilote, versions CUDA/SDK/FFmpeg, hash git, tableau des tests, VRAM avant/après, images/s) — **aucun chemin personnel ni e-mail** ; le fichier est versionnable mais n'est commité que sur demande.
5. Code de sortie : 0 = tout vert ; 1 = test rouge ; 2 = prérequis manquant.
`scripts/build.ps1` et `scripts/test.ps1` sont les deux autres points d'entrée (`-Preset`, `-Clean`, `-Verbose`), tous en PowerShell, aucun script Unix.

**Alternatives écartées.** 1. *GPU en CI via runner auto-hébergé* : voir ADR-029. 2. *Tests GPU lancés à la main sans rapport* : non traçable pour QA et pour la DoD.

**Conséquences.** Le rapport sert de preuve de merge. **Licence.** `nvidia-smi` fourni par le pilote.

### 15.13 ADR-032 — Packaging MVP : version portable (ZIP)

**Contexte.** ENH-030 : un dossier à lancer pour la démo, sans environnement de dev ; installeur « plus tard ».

**Décision.** `scripts/package.ps1 -Version x.y.z` produit `dist\Enhance-x.y.z-win64-portable.zip` contenant : `Enhance.exe`, DLL Qt (via `windeployqt --release --qmldir src/app/qml --no-translations --no-system-d3d-compiler --no-opengl-sw` + vérification que seuls les modules/plugins/styles prévus sont présents), DLL FFmpeg LGPL, `LICENSES\` (LGPLv3, GPLv3 si exigé par Qt, LGPLv2.1 FFmpeg, MIT/BSD des autres, OFL de la police), `THIRD-PARTY-LICENSES.txt` généré par `scripts/licences.ps1`, `NOTICE-SOURCES.txt` (où obtenir les sources de Qt/FFmpeg et la configuration de build), `LISEZ-MOI.txt` (prérequis : Windows 11, pilote ≥ 617.42, carte RTX 50), `Enhance.exe.manifest` (PerMonitorV2, `longPathAware`), les `.pdb` ailleurs (non livrés). Sans runtime VC++ redistribuable livré à part : `/MD` exige le **VC++ Redistributable 2015-2022+ (x64)** ; soit il est vérifié au lancement avec message clair, soit le CRT est livré en « app-local » (`vcruntime140*.dll`, `msvcp140*.dll` autorisées par la licence Microsoft : à valider par secdevops) — décision : **app-local** pour le portable. Aucune DLL NVIDIA tant que non validée ; aucune DLL CUDA (cudart statique). Contrôles de fin : `check-no-network.ps1`, somme SHA-256 du ZIP écrite à côté, test de lancement dans un dossier propre.

**Alternatives écartées.** 1. *Installeur dès le MVP* : valeur faible pour une démo au PO, signature et désinstallation à soigner ; reporté. 2. *Exe unique « single file »* : incompatible avec la liaison dynamique LGPL (DLL remplaçables).

**Conséquences.** Release = tag semver + `CHANGELOG.md` + ZIP + SHA-256, **sans publication** sans accord du PO. **Licence.** Voir `docs/security/licences.md` ; `windeployqt` est un outil de build non redistribué.

### 15.14 ADR-033 — Installeur (plus tard) : Inno Setup recommandé

**Contexte.** Après le MVP : installation propre, raccourcis, désinstallation, vérification du pilote au lancement. Toast Windows natifs (ADR-004, A5) demandent un AUMID lié à un raccourci du menu Démarrer.

**Décision.** Recommandation : **Inno Setup 6** (script texte lisible, installation par utilisateur sans droits administrateur, raccourci avec AUMID pour les toasts, désinstallation propre, signature intégrée). À reconfirmer par un ADR daté au moment de la story d'installeur ; la structure du dossier portable est conçue pour être reprise telle quelle.

**Alternatives écartées.** 1. *MSIX* : bon pour AUMID/mises à jour propres, mais **signature obligatoire** (certificat de confiance), virtualisation du registre/dossiers, chargement de DLL NVIDIA et accès fichiers plus délicats à valider, orienté Store. 2. *WiX (MSI)* : le plus « entreprise », mais XML verbeux et courbe d'apprentissage forte ; modèle de licence/financement des versions récentes à vérifier.

**Conséquences.** Prérequis vérifiés au lancement par l'application (ADR-009), pas par l'installeur. **Licence.** Inno Setup : licence propre permissive (gratuit, usage commercial autorisé, redistribution de l'outil non requise) ; l'installeur généré n'est pas soumis à la licence de l'outil.

### 15.15 ADR-034 — Signature de code (plus tard)

**Contexte.** Sans signature, SmartScreen avertit à l'ouverture du ZIP/de l'exe (MotW) : acceptable pour la démo au PO, pas pour une diffusion.

**Décision.** Pas de signature au MVP ; à étudier avec le PO avant toute diffusion : service de signature géré (Microsoft « Trusted/Artifact Signing », éligibilité par région et type d'organisation **[H]**), certificat OV/EV d'une autorité, sinon rien. Le script `package.ps1` prévoit un point d'entrée `-Sign` (désactivé) lisant le certificat depuis une variable d'environnement, jamais depuis un fichier du dépôt.

**Alternatives écartées.** 1. *Certificat auto-signé* : n'apporte aucune confiance chez l'utilisateur. 2. *Ne jamais signer* : limite la diffusion.

**Conséquences.** Aucun secret dans le dépôt ; coût annuel éventuel à valider par le PO. **Licence.** Sans objet.

### 15.16 ADR-035 — Dépôt : `.gitignore`, `.gitattributes`, identité, hygiène

**Contexte.** Règles non négociables du PO (identité noreply, config d'équipe jamais versionnée, rien de tiers commité).

**Décision (à appliquer à l'étape 5, pas avant).** Le `.gitignore` existant est **complété** (non remplacé) pour couvrir au minimum : `.claude/`, `CLAUDE.md`, `LISEZMOI-EQUIPE.md`, `KICKOFF.txt`, `_claude-setup/` ; `third_party/` ; `build/`, `out/`, `cmake-build-*/`, `CMakeUserPresets.json`, `.vs/`, `.vscode/` (hors `settings.json` partagés éventuels), `*.user` ; `vcpkg_installed/`, `.vcpkg-root`, `F:/vcpkg-cache` (hors dépôt) ; `dist/`, `*.zip`, `*.msix`, `*.exe`, `*.dll`, `*.lib`, `*.pdb`, `*.obj` ; `*.mp4 *.mkv *.mov *.avi *.webm *.mxf *.y4m *.yuv *.png/*.jpg` sous `tests/**/clips` ; dossier `clips/` ; `*.onnx`, `*.engine`, `*.plan`, `*.trt` ; `.env`, `*.pfx`, `*.pem`, `*.key` ; `*.log`, `logs/`, `.coverage`, `coverage/`. `docs/qa/resultats/*.md` autorisés. `.gitattributes` : `* text=auto eol=lf` sauf `*.ps1`, `*.bat`, `*.cmd`, `*.sln` en `crlf` ; `*.png *.ttf *.otf *.woff2` en `binary` (les `.svg` restent du texte). `.editorconfig` cohérent avec `.clang-format` (UTF-8 sans BOM, indentation 4, LF ; BOM autorisé pour les `.ps1` si Windows PowerShell 5 l'exige **[H]**).
Hygiène : `git config --local user.name "Carol MBONZIMA"` et `user.email "81617903+rolkapone@users.noreply.github.com"` ; `scripts/precommit-check.ps1` (hook local) vérifie l'e-mail, refuse les fichiers de la liste d'exclusion, les binaires hors liste blanche, les motifs de secrets et toute chaîne ressemblant à une adresse e-mail personnelle ; contrôle `git log --format='%an <%ae>' main..<branche>` avant chaque merge ; aucun `git add -A`.

**Alternatives écartées.** 1. *Se fier au `.gitignore` seul* : n'arrête pas un `add -f`. 2. *Hooks côté serveur* : pas de remote pour l'instant.

**Conséquences.** Le dossier `docs/` est versionné (décision actuelle) ; `CLAUDE.md` reste local. **Licence.** Sans objet.

### 15.17 ADR-036 — Arborescence de `src/` et cibles CMake

**Contexte.** Cohérence entre les couches du developer (§ 1) et les modules du gpu-engineer (`docs/gpu/architecture-pipeline.md` § 3.1) ; règle : une cible ne lie que la couche du dessous.

**Décision.**
```
src/
  core/               enh_core        (STATIC, C++20 pur, sans Qt)    include/enh/ + *.cpp
  platform/win/       enh_platform    (STATIC, Win32/COM, sans Qt)
  pipeline/
    api/              enh_pipeline_api (INTERFACE : en-têtes publics, Expected, PipelineError)
    core/             enh_pipeline_core (STATIC : Pipeline, BoundedQueue, PauseGate, plan() ; testé en CI)
    fake/             enh_pipeline_fake (STATIC : FakePipeline, FakeMediaProbe)
    ffmpeg/ cuda/ nvenc/ rtxvideo/ interp/ gpu_factory/   enh_pipeline_gpu (STATIC ; **seulement si ENH_WITH_GPU**)
  app/                enh_app         (STATIC : modèles, contrôleurs, adaptateurs Qt ; seulement si ENH_WITH_UI)
  app/qml/            module QML « Enhance.App » (qt_add_qml_module), Theme
  app/i18n/           enhance_fr.ts
  main.cpp            enhance (exécutable WIN32) : racine de composition (pipeline réel ou faux selon ENH_WITH_GPU)
tests/
  unit/core/ unit/pipeline/ unit/app/ unit/qml/   (CTest label unit / ui)
  gpu/                                            (CTest label gpu, exclus de la CI)
  quality/                                        (scripts PSNR/SSIM/VMAF)
cmake/  (EnhNvidia.cmake, EnhWarnings.cmake, triplets/, toolchain)
vcpkg/overlay-ports/  vcpkg.json  vcpkg-configuration.json  CMakePresets.json
scripts/  (build.ps1 test.ps1 test-gpu.ps1 package.ps1 format.ps1 licences.ps1 fetch-ffmpeg.ps1 check-*.ps1 precommit-check.ps1)
```
Les noms `pipeline/api` et `pipeline/impl` du gpu-engineer sont conservés (`pipeline/ffmpeg|cuda|nvenc|rtxvideo|interp` = implémentation). Règles : `enh_core` ne lie que `enh_pipeline_api` ; `enh_app` lie `enh_core` (+ Qt) ; seul `enhance` lie le pipeline concret ; les cibles `*_gpu` n'existent pas en monde sans GPU (même `target_link_libraries` partout via une cible `enh_pipeline_impl` qui désigne soit `enh_pipeline_fake`, soit `enh_pipeline_gpu`).

**Alternatives écartées.** 1. *Une seule bibliothèque `libenhance`* : casse la règle de dépendance et l'isolement CI sans GPU. 2. *Un dépôt par module* : surdimensionné.

**Conséquences.** Le gpu-engineer et le developer peuvent travailler sur la même branche sans conflit de CMake (un `CMakeLists.txt` par dossier). **Licence.** Sans objet.

### 15.18 Installation : ce que fait le PO, ce que fait devops (étape 5)

**À installer par le PO (comptes, gros téléchargements, choix manuels).** Rien n'est à faire avant validation de cette section.
1. **SDK NVIDIA** (compte NVIDIA Developer) : RTX Video SDK, Video Codec SDK ≥ 13.0, Optical Flow SDK ≥ 5.0, TensorRT 10.x (ZIP Windows pour CUDA 13) à poser dans `F:\P153\third_party\nvidia\<sdk>\` (liens dans `docs/gpu/spike.md` §2.1) ; noter les versions.
2. **CUDA Toolkit 13.x** (version précisée par devops après lecture des notes VS 2026) : installeur NVIDIA, **mode personnalisé, décocher « Driver »** pour garder le pilote 617.42 (`winget show Nvidia.CUDA --versions` liste les versions, mais winget ne permet pas de décocher le pilote : utiliser l'installeur graphique). Vérification : `nvcc --version` et `echo $env:CUDA_PATH`.
3. **Composants Visual Studio 2026** (Visual Studio Installer > Modifier) : « Développement Desktop C++ » (déjà là) + « Outils CMake C++ pour Windows » + « Outils C++ Clang pour Windows » (clang-format/tidy) ; **optionnel (repli ADR-021)** « Outils de build MSVC v143 - VS 2022 C++ x64/x86 ». Vérification : `& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`.
4. **Espace disque** : prévoir ≈ 40 Gio libres (Qt temporaire 25 Gio, caches, SDK, TensorRT ~ 4 Gio).
5. Facultatif (devops peut le faire) : `winget install -e --id Git.Git` si absent.

**Installé/déposé par devops à l'étape 5 (sans compte, après accord du PO).**
- vcpkg : `git clone https://github.com/microsoft/vcpkg C:\Tools\vcpkg` ; `git -C C:\Tools\vcpkg checkout <sha de la baseline>` ; `C:\Tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics` ; variables utilisateur `VCPKG_ROOT=C:\Tools\vcpkg`, `VCPKG_DISABLE_METRICS=1`, `VCPKG_DEFAULT_BINARY_CACHE=F:\vcpkg-cache` (`[Environment]::SetEnvironmentVariable(..., 'User')`).
- Premier `scripts/build.ps1 -Preset dev` : compilation de Qt (≈ 1 h 20), GoogleTest, spdlog, nlohmann-json, tl-expected.
- FFmpeg BtbN `lgpl-shared` : `scripts/fetch-ffmpeg.ps1` (SHA-256 vérifié) vers `third_party/ffmpeg/lgpl-shared/` ; FFmpeg « outil » GPL pour la QA : hors dépôt, avec l'accord du PO.
- LLVM/clang-format à la version épinglée (`winget install -e --id LLVM.LLVM --version <x.y.z>` si le composant VS ne suffit pas), OpenCppCoverage (`winget install -e --id OpenCppCoverage.OpenCppCoverage`).
- `git init` côté configuration locale (identité vérifiée), `.gitignore` complété, hook de pré-commit, presets, workflow CI, scripts, README.
- `CMakeUserPresets.json` local (chemins SDK) ; **aucune modification du PATH** (les scripts utilisent des chemins absolus du CMake de VS).

---

## 16. Registre des briques proposées par ce document

À valider par secdevops et à reporter dans `docs/security/licences.md`.

| Brique | Usage | Licence | Redistribuée |
|---|---|---|---|
| Qt 6 (Core, Gui, Qml, Quick, Quick Controls Basic/Templates, Quick Effects, Quick Dialogs, Svg) | Front | LGPLv3 (liaison dynamique) | oui (DLL) |
| Qt Test, Qt Quick Test | Tests | LGPLv3 / GPLv3 | non |
| spdlog | Journal | MIT | oui (statique) |
| nlohmann/json | Préréglages, persistance | MIT | oui (en-têtes) |
| tl::expected | Gestion d'erreurs | CC0-1.0 | oui (en-tête) |
| GoogleTest | Tests du cœur | BSD-3-Clause | non |
| Police (Segoe UI Variable système ou Inter) | Typographie | système Windows / OFL-1.1 | Inter seulement |

## 17. Risques

| # | Risque | Parade |
|---|---|---|
| A1 | Mica / fenêtre transparente instable sous Qt Quick + Direct3D 11 | Prototype dans ENH-004 ; repli fond opaque teinté par les tokens |
| A2 | Saccades de l'interface quand CUDA/NVENC saturent le GPU | *Animators* sur le thread de rendu, progression regroupée, mesure QA dès le premier pipeline réel ; priorité de flux CUDA basse côté gpu-engineer |
| A3 | Qt officiel lié à QtNetwork (contraire à ENH-028) | Qt compilé sans `qml-network` (ADR devops) ; sinon liste blanche documentée validée par secdevops |
| A4 | Accessibilité Qt Quick moins automatique que WinUI (contraste élevé, régions vives) | `SystemAppearance` en C++, `Accessible.announce()`, tests Narrateur à chaque story `[UI]` |
| A5 | Notification de fin (toast) impossible en version portable sans AUMID | Décision dans ENH-019 ; repli dans l'app + barre des tâches |
| A6 | Chemins longs non gérés par une brique tierce (FFmpeg) | Test commun developer/gpu-engineer sur chemin > 260 caractères |
| A7 | Courbe d'apprentissage QML + modèle objet Qt pour le PO | Cœur en C++ standard, Qt confiné à la présentation ; rubrique « À retenir en C++ » |

## 18. Questions ouvertes

1. **PO** : valider ADR-001 (Qt Quick).
2. **ux-designer** : « Rétablir » après annulation = « Reprendre depuis le début » ; au redémarrage, jobs interrompus remis « En attente » (MVP) ; progression sur l'icône de la barre des tâches ; suffixe de sortie quand la source est déjà en 4K ou à ≥ 50 i/s (`_Enhance_4K60` reste-t-il exact ?).
3. ~~**gpu-engineer** : accord sur l'interface du § 12, l'alias `enh::Expected`, le `FakePipeline`, `plan()` sans GPU, le passage des chemins longs à FFmpeg.~~ **Tranché le 2026-10-21** (§ 12, ADR-047, ADR-049, ADR-051). Reste pour l'ux-designer : texte P-08 du code `SourceUnreadable` (fichier verrouillé) et libellé de l'étape `PreparingModels` (premier lancement, 1–3 min **[H]**).
4. **devops** : source de Qt (vcpkg avec options vs installeur officiel), version exacte, rendu QML en CI.

### 15.18 ADR-037 — Amendement d'ADR-036 : `enh_common`, `enh_pipeline_ffmpeg`, `enhance-bench`, pont RTX Video

**Contexte.** Demandes du gpu-engineer (`docs/gpu/ENH-008.md`, `ENH-040.md`, `ENH-047.md`), sprint 1.

**Décision.**
1. **`enh_common`** (`src/common/`, INTERFACE, en-têtes seuls) : porte `enh/expected.hpp` (alias `enh::Expected`, `tl::expected`) et `ENH_HAVE_EXPECTED`. Lié par `enh_pipeline_api` (donc tout le pipeline) et par `enh_core`. Le pipeline ne dépend jamais d'`enh_core`. Déplacement fait sur `main` (commit `build:`) avant les branches.
2. **`enh_pipeline_ffmpeg`** (`src/pipeline/ffmpeg/`, STATIC, **sans CUDA**) : accepté. Démux, AVIO, décodage logiciel ; construite dès que la feature vcpkg `ffmpeg` est présente, **indépendamment de `ENH_WITH_GPU`** (testable en CI si FFmpeg y est installé, sinon cible absente et tests sautés ; `ENH_WITH_GPU=OFF` compile toujours). Ne lie que `enh_pipeline_api`. `enh_pipeline_gpu` la lie et y ajoute NVDEC/CUDA ; `enh_pipeline_fake` ne la lie pas.
3. **`enhance-bench`** (`src/tools/bench/`, exécutable console) : construit seulement si `ENH_GPU_EFFECTIVE`, lie `enh_pipeline_impl`, jamais livré dans le ZIP/installeur (outil de dev appelé par `test-gpu.ps1`).
4. **`enh_rtxvideo_bridge`** (`src/pipeline/rtxvideo_bridge/`, SHARED, seulement si `ENH_GPU_EFFECTIVE`) : DLL séparée avec `.def`, CRT statique `/MT` (`MSVC_RUNTIME_LIBRARY MultiThreaded`, dérogation locale au `/MD` de l'ADR-020), sans cudart, sans dépendance vers `enh_*` ; chargée dynamiquement par `enh_pipeline_gpu` (non liée à la compilation), livrée à côté de `enhance.exe` ; licence à consigner par secdevops.

**Conséquences.** La règle « une cible ne lie que la couche du dessous » est inchangée. **Alternatives écartées.** `expected.hpp` dans `enh_pipeline_api` : force `enh_core` à en dépendre pour un utilitaire hors pipeline ; FFmpeg seulement dans `enh_pipeline_gpu` : démux non testable en CI.

**Amendement 1 (ENH-008, 2026-10-22, gpu-engineer ; revue ENH-008 C3).** Constaté à l'implémentation, décidé ainsi :
1. **`enh_pipeline_stage`** (`src/pipeline/stage/include/enh/pipeline/stage/`, INTERFACE, en-têtes seuls) : interfaces d'étage **internes** au pipeline (`VideoFrame`, `FrameBuffer`, `Packet`, `IFrameSink`, `IDemuxer`, `IDecoder`). Elle lie `enh_pipeline_api` (pour `PipelineError`). `enh_core` et `enh_app` ne lient que `enh_pipeline_api` et ne voient donc pas ces en-têtes ; **nuance** : l'exécutable `enhance` (racine de composition) les voit par transitivité (`enh_pipeline_impl` → `enh_pipeline_gpu` / `enh_pipeline_fake` → `enh_pipeline_core` en `PUBLIC` → `enh_pipeline_stage`). Ce n'est pas gênant aujourd'hui (`main.cpp` ne les inclut pas) ; passer ces liaisons en `PRIVATE` est à faire avec la fabrique du pipeline (sans changement de CMake dans ENH-008). Écarté : les laisser dans `enh_pipeline_api/enh/pipeline/stage/`, qui exposait des types internes à l'application.
2. **`enh_pipeline_core`** porte aussi les briques pures C++ / Win32 réutilisées par tous les étages : `checkedMul` et les bornes SEC-13, la disposition NV12 / P010, `selectDecoder`, `Win32File` (ADR-049), le pool d'images CPU, la boucle de décodage et le journal du pipeline (`logLine` / `setLogSink`, branché sur `enh::log` par l'application, ADR-007). Elle lie `enh_pipeline_stage`.
3. **`enh_pipeline_ffmpeg` lie `enh_pipeline_core`** (et non `enh_pipeline_api` seule) : `Win32File`, bornes et journal sont dans `core`. La règle « une cible ne lie que les couches du dessous » est respectée : api ← stage ← core ← ffmpeg ← gpu.
4. Le point 2 de la décision initiale (« construite dès que la feature vcpkg `ffmpeg` est présente ») se lit, en phase de développement, « dès que FFmpeg est trouvé » : `cmake/EnhFfmpeg.cmake` définit `Enhance::ffmpeg` depuis `ENH_FFMPEG_ROOT` (défaut `third_party/ffmpeg/lgpl-shared`, ADR-025). Sans FFmpeg, la cible et ses tests sont absents et la CI compile (vérifié).

### 15.19 ADR-053 — Mica avec Qt Quick (risque A1) et deploiement local de Qt (ENH-004)

**Contexte.** ENH-004 devait prouver le Mica avec une fenetre Qt Quick transparente (Direct3D 11), ou consigner un repli opaque (risque A1). Le Qt de vcpkg ne fournit pas `windeployqt`.

**Decision.**
1. **Mica retenu (prototype reussi sur la RTX 5070, Windows 11 build 26200).** Fenetre creee avec `QQuickWindow::setDefaultAlphaBuffer(true)` (Qt passe alors par une surface DirectComposition en alpha premultiplie), `window.color: "transparent"`, `DWMWA_SYSTEMBACKDROP_TYPE = DWMSBT_MAINWINDOW`, `DwmExtendFrameIntoClientArea(-1)`, `DWMWA_USE_IMMERSIVE_DARK_MODE` selon le theme. Point dur trouve : la surface de redirection GDI de la fenetre est blanche par defaut et masque le Mica derriere la surface DirectComposition ; `WM_ERASEBKGND` est donc intercepte (filtre natif de `WindowController`) pour peindre en **noir GDI**, que DWM rend transparent dans un cadre etendu. Encapsule dans `enh_platform` (`applyWindowChrome`, `handleBackdropMessage`, `WindowCloak` RAII).
2. **Repli opaque automatique** (`RenderPolicy.micaUnavailable`) : Windows < 22H2 (build 22621), refus de DWM (`DwmSetWindowAttribute` en echec), option `--no-mica` ; aussi « Effets de transparence » coupes ou contraste eleve (fond `bgWindow`, `DWMWA_CAPTION_COLOR = bgWindow` sans couture).
3. **Ouverture sans flash** : fenetre masquee par `DWMWA_CLOAK` jusqu'au premier `frameSwapped` (filet de securite 1,5 s).
4. **Deploiement de developpement** : `cmake/EnhDeployQt.cmake` (post-build) copie a cote de `enhance.exe` une liste fermee de plugins (`platforms/qwindows`, `iconengines/qsvgicon`, `imageformats/qsvg`), les modules QML trouves par `qmlimportscanner` et leurs DLL (`file(GET_RUNTIME_DEPENDENCIES)`) ; echoue si un `Qt6Network*.dll` apparait. Le paquet livrable reste l'affaire d'ENH-030.

**Alternatives ecartees.** 1. *Repli opaque d'office* : le Mica fonctionne, le repli reste disponible. 2. *Barre de titre dessinee par l'application* : contraire a la note UX (boutons et snap layouts natifs). 3. *Qt officiel pour `windeployqt`* : importe `Qt6Network` (ADR-024).

**Consequences.** Si « Afficher la couleur d'accentuation sur les barres de titre » est active dans Windows, DWM peint la barre de titre a la couleur d'accent (choix de l'utilisateur, hors de notre controle). Licences : aucune nouvelle brique (API Win32/DWM du systeme).
