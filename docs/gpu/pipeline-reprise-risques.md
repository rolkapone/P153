> Extrait de l'ancien `docs/gpu/architecture-pipeline.md` (ménage docs du 2026-10-29), texte inchangé. Sommaire : [architecture-pipeline.md](architecture-pipeline.md).

## 7. Reprise après plantage (prévue, hors MVP — ENH-038)

Conditions posées dès le MVP pour ne rien réécrire :
- `JobSpec` sérialisable avec `schema_version` ; numérotation déterministe des images de sortie (`CadencePlanner` pur) ;
- l'encodeur sait forcer une IDR (`NV_ENC_PIC_FLAG_FORCEIDR`, `nvEncodeAPI.h` l. 680) à une image donnée ;
- le démuxeur sait chercher (`seek`) ; `TimeRange` existe déjà (aperçu).
Mécanique prévue : segments fermés de ~10 s (IDR au début de chaque segment), écrits en MKV/MP4 fragmenté dans un dossier temporaire du job + journal `resume.json` (dernier segment complet, image de sortie, empreinte des réglages). Au redémarrage : on reprend au segment suivant (recherche à l'image clé précédente, décodage jusqu'à l'image exacte, une image source de plus pour l'interpolation), puis remux final sans réencodage en MP4 `faststart`. La même mécanique permettra une pause qui libère la VRAM (ADR-046).

## 8. Extensibilité (vision long terme)

| Besoin | Point d'extension prévu |
|---|---|
| SDR → HDR10 (ENH-032) | étage `IHdrConverter` = **TrueHDR du RTX Video SDK** (interface CUDA, entrée RGB SDR, sortie ABGR10 ou FP16, pilote ≥ r570 ; mesuré 737 i/s et 164 Mio en 4K **[M]**) inséré après la SR ; sortie P010 BT.2020/PQ ; métadonnées HDR10 (mastering display, MaxCLL/MaxFALL) écrites dans le flux NVENC (`outputMasteringDisplay` / `outputMaxCll` en HEVC et en AV1, structures `MASTERING_DISPLAY_INFO` / `CONTENT_LIGHT_LEVEL` : `nvEncodeAPI.h` l. 1711-1728, 1972-1973, 2095-2096) **et** dans le conteneur (`mdcv`/`clli` en MP4, via les données annexes de `AVCodecParameters`) ; `DynamicRange::Hdr10` refusé au MVP (`UnsupportedOption`). Sources déjà HDR10 : au MVP, traitement en 10 bits avec conservation des métadonnées, à valider en QA (risque R7). |
| Aperçu de quelques secondes (ENH-036) | `JobSpec::range` : recherche, décodage à vide jusqu'au début, arrêt à la fin ; même pipeline |
| Avant / après (ENH-033) | `IFrameTap` optionnel sur la sortie du décodage et de la SR : copie asynchrone d'une image réduite vers un tampon épinglé, sur un flux dédié de basse priorité, au plus 1 image/s ; aucune synchro dans la boucle |
| Options avancées (ENH-035) | champs déjà présents dans `PipelineParams` (taille cible, débit, conteneur) ; validés par `plan()` (erreurs typées) |
| Préréglages personnalisés (ENH-034) | `PipelineParams` est une donnée ; identifiant et nom du préréglage restent côté application |
| Dossiers entiers (ENH-037) | côté application (N `JobSpec`) ; rien dans le pipeline |
| Autres cartes RTX 50 | `GpuCapabilities` (nombre de NVENC, AV1, VRAM) ; budget VRAM relatif à la VRAM libre ; encodage « split-frame » si ≥ 2 NVENC (`NV_ENC_SPLIT_ENCODE_MODE`, `nvEncodeAPI.h` l. 2232 ; sans objet sur la 5070, qui n'a qu'un NVENC) |

## 9. Risques

| # | Risque | Effet | Parade |
|---|---|---|---|
| R1 | Le RTX Video SDK n'expose pas d'interop CUDA (seulement D3D11/D3D12) ou n'a pas de mode « hors ligne » | Copie ou interop supplémentaire | **Levé (phase 2)** : interface CUDA native, pas d'interop D3D. **Risque lié découvert (B4)** : `nvsdk_ngx_s.lib` (compilée en CUDA 11.8) plante avec le `cudart` de CUDA 13 → isolement dans une DLL pont sans `cudart`, validé ; risque résiduel si NVIDIA change la lib. Autre point : le « NGX Update Module » du pilote peut télécharger des DLL (avis secdevops) |
| R2 | Format d'entrée/sortie du SDK limité à 8 bits | Perte du 10 bits | **Levé (phase 2)** : ABGR10 accepté en entrée et en sortie, même vitesse qu'en 8 bits |
| R3 | SR plus lente que 60 i/s sur la 5070 avec l'interpolation avant SR (60 images à agrandir) | Équilibré < 60 i/s | Formule ADR-044 + mesure des deux ordres ; décision PO n° 5 |
| R4 | Licence des poids RIFE refusée | Pas de RIFE | FRUC (qualité moindre, à mesurer) ; autre réseau sous licence claire |
| R5 | Un seul NVENC sur la 5070 (**confirmé**) | NVENC est le goulot de toute la chaîne : Équilibré ≈ 105 i/s, P7 HQ 37 (HEVC) / 52 (AV1), UHQ 29 / 38 **[M]** | Qualité max visée à ≥ 30 i/s seulement ; réglages P5 pour Équilibré |
| R6 | Charge GPU à 100 % → animations Qt Quick saccadées (ENH-020 exige 60 i/s d'interface) | Exigence design | Flux CUDA de priorité basse, ordonnancement GPU matériel Windows ; option de limitation (cédant entre deux images) ; mesure QA dès le premier pipeline complet |
| R7 | Sources entrelacées, HDR, 4:2:2/4:4:4, 12 bits | Rendu faux | Désentrelacement : `bwdif_cuda` de libavfilter si présent dans le build LGPL (à vérifier), sinon noyau CUDA maison ; 4:2:2/4:4:4 → décodage logiciel + conversion ; clips QA dédiés |
| R8 | Fichiers malformés faisant planter FFmpeg | Plantage du processus | `Expected` partout, ouvertures secondaires refusées (ADR-049), limites de taille (dimensions, nombre de pistes), fuzzing ENH-015 ; à terme, probe dans un processus séparé (à discuter avec secdevops) |
| R9 | `std::expected` absent en C++20 | Incohérence d'API | **Résolu** : alias unique `enh::Expected` (ADR-003 / ADR-047), `tl::expected` en C++20 |
| R10 | Construction du moteur TensorRT au premier lancement (minutes) | Premier job lent | Étape `BuildingModels` visible, cache local, invalidation sur changement de pilote |
| R11 | Mesure de VRAM par processus `N/A` en WDDM | Budgets mal vérifiés | `cuMemGetInfo` + DXGI dans le processus (cf. `spike.md` R4) |

## 10. À vérifier en phase 2 (fichiers à lire et citer)

**État au 2026-10-21 (fin de phase 2)** : levés, avec fichiers cités dans `spike-phase2.md` § P2.2 : NVENC (UHQ, HDR10 HEVC/AV1, `nvEncSetIOCudaStreams`, AV1 10 bits, 1 seul NVENC), RTX Video SDK (CUDA, formats RGB 8/10 bits, niveaux 0–4, TrueHDR, redistribution des DLL `rel`), FRUC (Blackwell via JIT PTX, NV12/ARGB 8 bits, instant arbitraire). **Restent ouverts** : `nvcuvid.h`/`cuviddec.h` et `AppDec` (FFmpeg absent), TensorRT/RIFE (pas d'ONNX), toute la liste FFmpeg ci-dessous, et le « NGX Update Module » (réseau, secdevops).

- Video Codec SDK : `Interface/nvEncodeAPI.h` (préréglages/UHQ, HDR10 HEVC/AV1, `nvEncSetIOCudaStreams`, mode asynchrone, capacités AV1 10 bits, nombre de NVENC), `Interface/nvcuvid.h` / `cuviddec.h` (formats de sortie), exemples `AppEncCuda`, `AppDec`.
- RTX Video SDK : en-têtes et guide (interop CUDA ou D3D, formats d'E/S, niveaux de qualité, SDR→HDR, mode hors ligne, redistribution des DLL).
- Optical Flow SDK : en-tête FRUC (pas temporel arbitraire ?, Blackwell, formats).
- TensorRT 10 : `NvInferRuntime.h` (gestion mémoire, `setDeviceMemory` pour mutualiser l'espace de travail), DLL à redistribuer.
- FFmpeg 8.x (build BtbN `lgpl-shared`) : `libavutil/hwcontext_cuda.h` (`AV_CUDA_USE_PRIMARY_CONTEXT`), présence de `bwdif_cuda`, `avformat_query_codec` pour MP4, écriture `av1C`/`mdcv`/`clli` en MP4 ; `libavformat/movenc.c` (réouverture du fichier de sortie par `io_open` pour `faststart`, ADR-049) ; `libavformat/avformat.h` (`io_open`/`io_close2`, `AVFMT_FLAG_CUSTOM_IO`) ; `libavutil/wchar_filename.h` (chemins longs du protocole `file:`, pour mémoire).
