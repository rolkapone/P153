# Enhance — Architecture : index

Statut : v0.1 du 2026-10-21 — **proposition** (developer), à valider par le PO (ADR-001 en particulier).
Périmètre de ce document : l'application **hors pipeline vidéo GPU** (interface, file d'attente, préréglages, persistance, journal, prérequis, fichiers). Le pipeline est décrit par le gpu-engineer dans `docs/gpu/architecture-pipeline.md` ; build, CI et packaging par devops (section dédiée en fin de document).

Numérotation des ADR, pour éviter les collisions entre auteurs :
- **ADR-001 à ADR-019** : application (developer) ;
- **ADR-020 à ADR-039** : build, dépendances, CI, packaging (devops) ;
- **ADR-040 et plus** : pipeline vidéo (gpu-engineer), y compris s'ils sont rédigés dans `docs/gpu/`.

Convention : **[H]** = hypothèse à confirmer (spike, prototype ou mesure).

---

Texte d'origine découpé le 2026-10-29 (ménage docs) : un fichier par ADR, sans réécriture. L'ancien `docs/architecture.md` et les ADR-040+ de `docs/gpu/architecture-pipeline.md` sont désormais ici.

## Schéma d'ensemble et autres sections

- Vue d'ensemble en couches (ex-§ 1) : [vue-ensemble.md](vue-ensemble.md)
- Ce que l'application attend de l'interface pipeline (ex-§ 12) : [interface-pipeline.md](interface-pipeline.md) ; pipeline vidéo (ex-§ 13) : [pipeline-video.md](pipeline-video.md) ; tests sans GPU (ex-§ 14) : [tests-sans-gpu.md](tests-sans-gpu.md)
- Build, dépendances, CI, packaging : intro et installation (ex-§ 15 hors ADR) : [build-packaging.md](build-packaging.md)
- Registre des briques, risques, questions ouvertes (ex-§ 16-18) : [registre-risques-questions.md](registre-risques-questions.md)
- Pipeline GPU (conception, VRAM, contrat d'API, risques) : [../gpu/architecture-pipeline.md](../gpu/architecture-pipeline.md)

## ADR

| ADR | Titre | Fichier |
|---|---|---|
| ADR-001 | Technologie du front : Qt 6 / Qt Quick (QML) | [ADR-001.md](ADR-001.md) |
| ADR-002 | Couches, modèle de threads et passage vers le thread UI | [ADR-002.md](ADR-002.md) |
| ADR-003 | Gestion des erreurs : `expected` et aucune exception aux frontières | [ADR-003.md](ADR-003.md) |
| ADR-004 | File d'attente et orchestration des jobs | [ADR-004.md](ADR-004.md) |
| ADR-005 | Préréglages en données JSON versionnées | [ADR-005.md](ADR-005.md) |
| ADR-006 | Persistance des réglages et de la file | [ADR-006.md](ADR-006.md) |
| ADR-007 | Journalisation locale | [ADR-007.md](ADR-007.md) |
| ADR-008 | Internationalisation (français d'abord) | [ADR-008.md](ADR-008.md) |
| ADR-009 | Détection des prérequis (carte, pilote) | [ADR-009.md](ADR-009.md) |
| ADR-010 | Fichiers, chemins et sortie | [ADR-010.md](ADR-010.md) |
| ADR-011 | (plage application réservée, aucun ADR rédigé) | — |
| ADR-012 | (plage application réservée, aucun ADR rédigé) | — |
| ADR-013 | (plage application réservée, aucun ADR rédigé) | — |
| ADR-014 | (plage application réservée, aucun ADR rédigé) | — |
| ADR-015 | (plage application réservée, aucun ADR rédigé) | — |
| ADR-016 | (plage application réservée, aucun ADR rédigé) | — |
| ADR-017 | (plage application réservée, aucun ADR rédigé) | — |
| ADR-018 | (plage application réservée, aucun ADR rédigé) | — |
| ADR-019 | Cité seulement par la plage d'en-tête, jamais rédigé (rien d'inventé) | — |
| ADR-020 | Toolchain : Visual Studio 2026 (MSVC v145), CMake, Ninja, presets | [ADR-020.md](ADR-020.md) |
| ADR-021 | CUDA Toolkit 13.x et compilateur hôte | [ADR-021.md](ADR-021.md) |
| ADR-022 | Dialecte C++20, avertissements, exceptions | [ADR-022.md](ADR-022.md) |
| ADR-023 | Dépendances : vcpkg en mode manifeste, baseline épinglée | [ADR-023.md](ADR-023.md) |
| ADR-024 | Qt 6 sans réseau : compilé via vcpkg, sans `Qt6Network` | [ADR-024.md](ADR-024.md) |
| ADR-025 | FFmpeg : LGPL, liaison dynamique | [ADR-025.md](ADR-025.md) |
| ADR-026 | SDK NVIDIA : `third_party/nvidia/`, détectés, jamais commités | [ADR-026.md](ADR-026.md) |
| ADR-027 | Preuve « zéro réseau » côté build | [ADR-027.md](ADR-027.md) |
| ADR-028 | Rendu QML en CI sans GPU | [ADR-028.md](ADR-028.md) |
| ADR-029 | CI GitHub Actions (`rolkapone/P153`, privé) | [ADR-029.md](ADR-029.md) |
| ADR-030 | Qualité : clang-format, clang-tidy, sanitizers, couverture | [ADR-030.md](ADR-030.md) |
| ADR-031 | Tests GPU locaux : `scripts/test-gpu.ps1` | [ADR-031.md](ADR-031.md) |
| ADR-032 | Packaging MVP : version portable (ZIP) | [ADR-032.md](ADR-032.md) |
| ADR-033 | Installeur (plus tard) : Inno Setup recommandé | [ADR-033.md](ADR-033.md) |
| ADR-034 | Signature de code (plus tard) | [ADR-034.md](ADR-034.md) |
| ADR-035 | Dépôt : `.gitignore`, `.gitattributes`, identité, hygiène | [ADR-035.md](ADR-035.md) |
| ADR-036 | Arborescence de `src/` et cibles CMake | [ADR-036.md](ADR-036.md) |
| ADR-037 | Amendement d'ADR-036 : `enh_common`, `enh_pipeline_ffmpeg`, `enhance-bench`, pont RTX Video | [ADR-037.md](ADR-037.md) |
| ADR-038 | Cité seulement par la plage d'en-tête, jamais rédigé (rien d'inventé) | — |
| ADR-039 | Cité seulement par la plage d'en-tête, jamais rédigé (rien d'inventé) | — |
| ADR-040 | Décodage : NVDEC via hwaccel FFmpeg (`AV_HWDEVICE_TYPE_CUDA`) | [ADR-040.md](ADR-040.md) |
| ADR-041 | Encodage : NVENC direct (API `nvEncodeAPI.h` du Video Codec SDK) | [ADR-041.md](ADR-041.md) |
| ADR-042 | Couleur et format pivot : noyaux CUDA maison, conversions explicites | [ADR-042.md](ADR-042.md) |
| ADR-043 | Brique d'interpolation : RIFE 4.x + TensorRT natif (cible), FRUC Optical Flow en repli | [ADR-043.md](ADR-043.md) |
| ADR-044 | Ordre : interpolation avant la super-résolution (provisoire, tranché par mesure) | [ADR-044.md](ADR-044.md) |
| ADR-045 | Concurrence : un thread par étage + flux CUDA + événements, files bornées | [ADR-045.md](ADR-045.md) |
| ADR-046 | Pause : VRAM conservée (MVP) | [ADR-046.md](ADR-046.md) |
| ADR-047 | Erreurs : `enh::Expected<T, PipelineError>` (alias de l'ADR-003), aucune exception aux frontières | [ADR-047.md](ADR-047.md) |
| ADR-048 | DLL NVIDIA chargées dynamiquement | [ADR-048.md](ADR-048.md) |
| ADR-049 | Fichiers : l'application possède le cycle de vie, le pipeline écrit au chemin exact reçu, E/S FFmpeg par `AVIOContext` Win32 maison | [ADR-049.md](ADR-049.md) |
| ADR-050 | Conteneur : MP4, bascule MKV (décision PO n° 2) | [ADR-050.md](ADR-050.md) |
| ADR-051 | Contrat d'API pipeline ↔ application (`JobHandle` RAII, observateur, progression, `plan()` pur) | [ADR-051.md](ADR-051.md) |
| ADR-052 | Pilote NVIDIA minimal : R610 (610.00) pour l'application, 617.42 comme pilote de référence des mesures | [ADR-052.md](ADR-052.md) |
| ADR-053 | Mica avec Qt Quick (risque A1) et deploiement local de Qt (ENH-004) | [ADR-053.md](ADR-053.md) |
