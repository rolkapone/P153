> Extrait de l'ancien `docs/architecture.md` (ménage docs du 2026-10-29), texte inchangé. Index : [architecture/index.md](index.md).

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
