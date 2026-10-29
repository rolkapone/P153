> Extrait de l'ancien `docs/architecture.md` (ménage docs du 2026-10-29), texte inchangé. Index : [architecture/index.md](index.md).

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
