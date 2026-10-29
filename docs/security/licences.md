# Enhance — Registre des licences

Statut : v1.0 du 2026-10-21 (secdevops), story ENH-006.
**Je ne suis pas juriste.** Ce registre résume ma lecture des textes réellement fournis ; tout point marqué « à confirmer par le PO » doit être tranché par lui (et, en cas de diffusion publique, idéalement relu par un juriste).

Hypothèse de publication (décision PO n° 4, consignée) : **Enhance est publié gratuitement, code source privé pour l'instant**. FFmpeg en **LGPL uniquement, en liaison dynamique** (aucun composant GPL lié, aucune option `--enable-nonfree`). Toute licence incompatible avec ce cadre est marquée « non ».

Légende :
- **Usage** : *build* (outil de compilation ou de test, jamais livré), *livré* (dans le ZIP / l'installeur), *pilote* (fourni par le pilote NVIDIA sur la machine de l'utilisateur, jamais livré par nous), *local* (machine du PO uniquement).
- **Redistribuable** : oui / non / à confirmer.
- **Verdict** : **OK** (utilisable dans l'usage indiqué), **À CONFIRMER** (bloquant pour la livraison tant que non tranché), **BLOQUÉ** (interdit dans l'usage demandé).

Règle générale : `third_party/` n'est **jamais** commité (aucun SDK, binaire, modèle). Le contenu livré est vérifié par `scripts/licences.ps1` (inventaire) et `scripts/check-no-network.ps1`.

---

## 1. Vue d'ensemble — briques sensibles

| Brique | Verdict livraison | En une ligne |
|---|---|---|
| RTX Video SDK 1.1.0 (`nvngx_vsr.dll`, `nvngx_truehdr.dll` ; `nvsdk_ngx_s.lib` liée statiquement dans `enh_rtxvideo_bridge.dll` depuis ENH-040) | **OK sous conditions** (+ 2 points à confirmer) | Art. 1.c : redistribution autorisée « incorporé [sous forme objet] dans une application » (DLL de fonction **et** bibliothèque NGX intégrée au pont) ; DLL `rel` seulement ; mention NVIDIA ; zéro réseau à prouver (SEC-01, porte ENH-041) |
| Video Codec SDK 13.1.15 | **OK** | Seuls les en-têtes (MIT) et éventuellement du code d'exemple (MIT) sont utilisés ; NVENC/NVDEC fournis par le pilote ; rien à redistribuer |
| Optical Flow SDK 5.0.7 — FRUC (`NvOFFRUC.dll`) | **BLOQUÉ** pour la livraison ; OK en local (**simple repli** depuis le 2026-10-21, SEC-04) | Art. 1.c : seul le code source d'exemple est distribuable ; la DLL FRUC ne l'est pas |
| TensorRT 11.4.0.106 « Enterprise » | **OK sous conditions** (local et livraison ; confirmations PO du 2026-10-21) | SLA SDK NVIDIA + Supplément TensorRT art. 2 : DLL d'exécution redistribuables, intégrées à Enhance, non modifiées, CLUF cohérent, `Acknowledgements.txt` livré (§2.4) |
| CUDA runtime 13.4 (`cudart_static.lib`) ; `cuda.lib` (chargeur de l'API pilote, lié dans le pont RTX Video depuis ENH-040) | **OK** pour `cudart_static.lib` ; **à confirmer** pour `cuda.lib` (§2.5, SEC-35) | EULA CUDA, Attachment A : `cudart_static.lib` distribuable ; `cuda.lib` (chargeur statique en CUDA 13) non listé |
| Poids RIFE 4.25 (Practical-RIFE, `flownet.pkl`) → ONNX **exporté par nous** | **OK sous conditions** (2026-10-21 ; 2 points à confirmer par le PO) | README de `hzwer/Practical-RIFE` (commit `bbfd2ea9`) : poids « under the same MIT license as this project » ; l'ONNX de vs-mlrt (dépôt GPL-3.0) est **écarté** de la livraison (§3, SEC-03) |
| FFmpeg LGPL partagé (BtbN puis vcpkg) | **OK** (BtbN : développement seulement ; vcpkg : livraison) | LGPL-2.1+, DLL séparées, sources et configuration à fournir |
| Qt 6.8+ LGPLv3, compilé sans réseau | **OK sous conditions** | DLL remplaçables, textes, sources + configuration de build, modules LGPL uniquement |
| CRT Visual C++ v145 app-local | **OK** | « Distributable Code » de Visual Studio (fichiers `Microsoft.VC145.CRT`) |
| Police Segoe UI Variable | **OK** (non livrée) | Police système Windows : utilisée, **jamais copiée** dans le livrable |
| Police Inter (si retenue) | **OK** | SIL OFL 1.1 : livrable avec `OFL.txt` |
| Jeu d'icônes | **À CONFIRMER** (choix ui-designer à venir) | Licences admises listées §5 ; SF Symbols interdit |

---

## 2. SDK NVIDIA (posés par le PO dans `third_party/nvidia/`, jamais commités)

### 2.1 RTX Video SDK 1.1.0

- Fichier lu : `third_party/nvidia/rtx-video-sdk/NVIDIA_RTX_Video_SDK_License.pdf` = « NVIDIA RTX SDKs LICENSE » (v. 23 février 2024) + « NVIDIA RTX Supplement » (v. 20 décembre 2023). Guide : `doc/NVIDIA_RTX_Video_SDK_Programming_Guide.pdf`, chap. 4 « Distributing RTX Video Features with Your Application ».
- Contenu binaire : `bin/Windows/x64/rel/nvngx_vsr.dll`, `nvngx_truehdr.dll` (et `dev/`, `arm64/`) ; bibliothèques `lib/Windows/x64/nvsdk_ngx_{d,s}[_dbg].lib`.

| Point | Texte | Lecture |
|---|---|---|
| Droit de distribuer | Art. 1.c : « Distribute any software and materials within the SDK, other than developer tools provided for your internal use, as incorporated in object code format into a software application subject to the distribution requirements » | **Oui** pour `nvngx_vsr.dll` (et `nvngx_truehdr.dll` quand ENH-032 arrivera), livrées avec Enhance. Le guide (chap. 4) prévoit explicitement de les installer dans le dossier de l'exécutable |
| DLL `dev` | Guide chap. 4 : `dev` = développement, « usually watermarked » ; `rel` = « for release » | Livrer **uniquement** `bin/Windows/x64/rel/`. Si un filigrane « PreRelease » apparaît, contacter NVIDIA avant toute diffusion (guide chap. 4) |
| Conditions | Art. 2.a : fonctionnalités additionnelles substantielles ; 2.c : distribuer sous des conditions au moins aussi protectrices (pas d'ingénierie inverse, etc.) et protéger la vie privée et la sécurité des utilisateurs ; 2.d : signaler à NVIDIA toute violation connue | Enhance doit avoir un CLUF qui reprend ces protections pour les composants NVIDIA (SEC-21) ; la confidentialité est cohérente avec « zéro réseau » |
| Interdits | Art. 4.a pas d'ingénierie inverse ; 4.b pas de distribution autonome ; 4.c pas de mention « sponsorisé / approuvé par NVIDIA » ; 4.e le SDK ne doit pas devenir soumis à une licence open source (exemples : obligation de fournir les sources, de permettre les dérivés, d'être redistribuable gratuitement) | La liaison **dynamique** avec Qt/FFmpeg LGPL ne soumet pas le SDK à la LGPL. **Enhance ne doit pas être publié sous GPL** (à confirmer par le PO si le code devient public : licence permissive ou propriétaire) |
| Matériel | Supplément art. 1 : applications interopérables avec le matériel NVIDIA ; NGX licencié pour des systèmes à GPU NVIDIA | Conforme (RTX 50 exigée) |
| Mises à jour | Art. 5 : « NVIDIA may ... automatically update the SDK or other software in the system, except for those updates that you may opt-out via the SDK API » ; guide chap. 1 p. 7 : le NGX Update Module télécharge et remplace la DLL | **Conflit potentiel avec « zéro réseau »** → SEC-01 (threat model). Aucune API d'opt-out documentée trouvée dans les en-têtes ; `NVSDK_NGX_UpdateFeature` ne doit jamais être appelée |
| Notification | Supplément art. 4 : obligation de **notifier NVIDIA avant la sortie commerciale** d'une application qui intègre le DLSS SDK ou le **NGX SDK** (formulaire indiqué dans le texte) | Le RTX Video SDK repose sur NGX (`nvsdk_ngx_*.lib`). **À confirmer par le PO** : (a) le RTX Video SDK est-il visé par « NGX SDK » ? (b) une publication **gratuite** est-elle une « commercial release » ? Recommandation prudente : notifier NVIDIA avant toute diffusion publique (gratuit, sans contrepartie) |
| Attribution | Supplément art. 7.1(c) : pour un SDK autre que DLSS/NGX, attribuer l'usage du SDK et inclure les marques NVIDIA dans l'écran de crédits ou, à défaut, dans la documentation ; 7.1(b) pour NGX : marques dans l'écran « À propos » ; 7.2(c) : tout usage des marques en **marketing** soumis à approbation écrite préalable | Prévoir dans « À propos » et dans `LISEZ-MOI.txt` : « Utilise le NVIDIA RTX Video SDK. NVIDIA et RTX sont des marques de NVIDIA Corporation. » **Pas de logo NVIDIA** ni d'usage marketing sans accord. **À confirmer par le PO** : forme exacte de la mention |
| Codecs | Supplément art. 5 : licences brevets audio/vidéo à la charge du développeur | Voir §7 (brevets) |
| **`nvsdk_ngx_s.lib` liée statiquement** (ENH-040, 2026-10-24) | Art. 1.c (cité ci-dessus) : « any software and materials within the SDK, other than developer tools […], as incorporated in object code format into a software application » | **Oui** : la bibliothèque NGX (variante `_s`, CRT statique) est intégrée sous forme objet dans `enh_rtxvideo_bridge.dll`, qui n'est utilisable que par Enhance (API `rvb_*` propre, chargée par chemin absolu) ; ce n'est pas un outil de développement ; jamais commitée (`third_party/`), seul le binaire du pont sera livré. Conséquences : (1) le pont **contient du code NVIDIA** : le CLUF d'Enhance (SEC-21, art. 2.c) doit le couvrir comme les DLL NVIDIA ; (2) le pont **ré-exporte 27 fonctions `NVSDK_NGX_*`** (directives d'export de la `.lib`, SEC-32) : pas une « distribution autonome » au sens de l'art. 4.b à mon avis (le pont n'a pas d'autre usage qu'Enhance), **à confirmer par le PO** si une lecture prudente est souhaitée (signalement § 12.3 d'ENH-040 déjà prêt) ; (3) art. 4.a (ingénierie inverse) : l'équipe s'est limitée à lister les symboles importés par la `.lib` (`dumpbin /symbols`, information d'édition de liens) et à observer un plantage ; ne pas aller plus loin (pas de désassemblage) |
| Signalement à NVIDIA (§ 12 de `docs/gpu/ENH-040.md`) | Art. 19 (contact) ; pas d'obligation de signaler un défaut (l'art. 2.d vise les **violations** de licence) | **Non envoyé** à ce jour (action du PO, facultative) ; à compléter par le constat SEC-31 (magasin NGX inscriptible par tous). Non bloquant pour la licence |
| CRT Visual C++ **statique** (`/MT`) dans le pont | Licence Visual Studio, « Distributable Code » (bibliothèques d'exécution liées dans l'application) | **OK** : dérogation locale au `/MD` (ADR-037), aucune DLL CRT supplémentaire livrée |

**Verdict : OK pour livrer `rel/nvngx_vsr.dll`**, sous conditions (CLUF, mention, DLL `rel`, SEC-01 levé) ; points à confirmer : notification NGX, forme de l'attribution.

### 2.2 Video Codec SDK 13.1.15

- Fichiers lus : `video-codec-sdk/License.pdf` (« NVIDIA DesignWorks SDK License », v. 10 mai 2022, Release 13.1), `NOTICES.txt` ; en-têtes `Interface/nvEncodeAPI.h`, `nvcuvid.h`, `cuviddec.h` ; exemples `Samples/`.

| Point | Lecture |
|---|---|
| En-têtes `Interface/*.h` | Licence **MIT** en tête de fichier (« This copyright notice applies to this header file only ») : utilisables librement, mention de copyright à conserver dans les sources |
| Code d'exemple (`Samples/NvCodec`, `Samples/Utils`) | En-têtes également sous MIT ; la licence DesignWorks (art. 1.b, 1.c, 2.c) autorise modifications et distribution **sous forme compilée** dans une application, avec la mention « This software contains source code provided by NVIDIA Corporation. » dans les sources dérivées distribuées |
| Bibliothèques `Lib/win/x64` (stubs d'import) | Non nécessaires (chargement dynamique ADR-048) ; non livrées |
| DLL NVENC / NVDEC | `nvEncodeAPI64.dll`, `nvcuvid.dll` : **fournies par le pilote**, jamais livrées |
| `NOTICES.txt` | Le SDK inclut des portions de FFmpeg sous **LGPLv3** (exemples `FFmpegDemuxer`, etc.) : si du code d'exemple lié à FFmpeg est repris, il reste soumis à la LGPL pour ces portions |
| Interdits | Art. 4.a à 4.e (identiques au RTX SDK, dont 4.e « pas de licence open source sur le SDK ») |

**Verdict : OK** — rien de NVIDIA n'est redistribué depuis ce SDK hormis du code compilé dérivé d'exemples MIT.

### 2.3 Optical Flow SDK 5.0.7 (FRUC)

- Fichiers lus : `optical-flow-sdk/LicenseAgreement.pdf` (« NVIDIA DesignWorks SDK License »), `Read_Me.pdf`, `Release_Notes.pdf`, `Deprecation_Notices.pdf`, `Doc/NVOFA_FRUC.pdf`, `NvOFTracker/NOTICES.txt`, `Common/External/FreeImage/license-fi.txt`.

| Point | Lecture |
|---|---|
| Droit de distribuer | Art. 1.c : « Distribute any **sample source code** delivered in the SDK and derivative works of such sample source code, as incorporated in object code format into a software application » — **aucun droit** de distribuer les binaires du SDK |
| `NvOFFRUC.dll` (`NvOFFRUC/NvOFFRUCSample/bin/win64/`) | Binaire fourni dans un dossier d'exemple, mais **ce n'est pas du code source** : non couvert par l'art. 1.c → **non redistribuable** en l'état de ma lecture. Aucun texte de `Read_Me.pdf` ni de `NVOFA_FRUC.pdf` n'accorde de droit de redistribution (recherche « distribut » sans résultat pertinent) |
| Dépendance | `NvOFFRUC.dll` importe `cudart64_110.dll` (CUDA 11, vérifié par analyse des imports) : il faudrait aussi livrer ce runtime (redistribuable selon l'EULA CUDA 11, à prendre dans un CUDA Toolkit 11, pas dans le dossier d'exemple) |
| Matériel | Moteur OFA présent sur Blackwell ; **avis NVIDIA** (`Deprecation_Notices.pdf`) : le matériel Optical Flow **disparaît après Blackwell** — brique sans avenir au-delà des RTX 50 |
| `nvofapi64.dll` | Fournie par le pilote |
| Tiers inclus | NvOFTracker : OpenCV (BSD-3), YOLOv3 (MIT) ; FreeImage (FIPL ou GPL, au choix) dans `Common/External` : **utilisés par les exemples seulement**, ne pas lier à Enhance |

**Verdict : BLOQUÉ pour la livraison** (OK pour une utilisation locale par le PO et le spike). Levée possible : accord écrit de NVIDIA (contact de la licence) — **à décider par le PO**.

### 2.4 TensorRT 11.4.0.106 « Enterprise » (CUDA 13.4)

**Texte de référence (mise à jour du 2026-10-21)** : *License Agreement for NVIDIA Software Development Kits* (v. 24 mai 2021) + *TensorRT Supplement* (v. 25 février 2021), page `https://docs.nvidia.com/deeplearning/tensorrt/latest/reference/sla.html` indiquée par le PO, récupérée par l'orchestrateur le 2026-10-21 (aucun téléchargement par secdevops). Jeu de documentation **11.4.0** (`theme_switcher_version_match = '11.4.0'`, « Last updated on Oct 05, 2026 »). SHA-256 du HTML récupéré : `ee9cc052ec5f03616f1bd9777cea89dda1b8f5413e7a1948b6f31b64773746c4`. Copie PDF conservée hors dépôt (ignorée par git via `third_party/`) : `third_party/nvidia/_licences/TensorRT-SLA_2026-10-21.pdf`, SHA-256 `360a76c3f00438c40310effaba93bd25316adf939597b8aa4815f19d6032a890` (vérifié par secdevops le 2026-10-21), imprimée depuis ce HTML.

- Archive : `_archives/TensorRT-Enterprise-11.4.0.106-Windows-amd64-cuda-13.4-Release-external.zip` (70 entrées) : aucun fichier de licence ; `doc/README.txt` renvoie explicitement vers cette page SLA, donc c'est bien le texte qui s'applique à l'archive (confiance élevée).
- **« Enterprise »** : le mot n'apparaît pas dans le SLA. Dans `include/NvInferVersion.h`, les macros `TRT_MAJOR_ENTERPRISE`… définissent `NV_TENSORRT_MAJOR`, à côté de `NV_TENSORRT_LWS_*` (édition allégée / « safety ») : « Enterprise » désigne vraisemblablement **l'édition standard complète** de TensorRT (par opposition à TensorRT for RTX et à l'édition safety), et non un produit sous abonnement. Confiance **moyenne** : déduction tirée des en-têtes et du nom de l'archive, pas d'un texte de licence. « Release-external » = version publique (pas une pré-version au sens de l'art. 1.4).
- `doc/Acknowledgements.txt` : l'avertissement « LicenseRef-NvidiaProprietary … without an express license agreement … strictly prohibited » est cohérent avec le SLA (le SLA **est** cet accord exprès). Le fichier reproduit les licences des composants tiers (Abseil, Protobuf, Flatbuffers : Apache-2.0 / BSD ; Caffe : BSD ; half.h, jQuery, pybind11 : MIT ; OpenSSL ; Boost Beast ; yaml-cpp, etc.), qui s'appliquent selon l'art. 1.6 : **à livrer tel quel** avec les DLL (Apache-2.0 §4(d) impose de joindre les avis). Une partie ne concerne que les outils Python / exemples, mais on livre le fichier entier par prudence.
- En-têtes `include/*.h` : Apache-2.0 (SPDX). Aucune DLL TensorRT n'importe d'API réseau (vérifié précédemment par analyse des imports).

| Question | Réponse | Article |
|---|---|---|
| Usage local (développement, spike, machine du PO) | **Autorisé** : « Install and use the SDK », pour des applications destinées à des systèmes équipés de GPU NVIDIA (cas d'Enhance) | SLA 1.1 (i) ; Supplément 1 |
| DLL redistribuables | « the runtime files .so and .dll » : `nvinfer_11.dll`, `nvinfer_builder_resource_sm120_11.dll` (et `…_ptx_11.dll` si nécessaire), `nvonnxparser_11.dll`, `nvinfer_plugin_11.dll` si utilisé. `nvinfer_lean_11.dll`, `nvinfer_dispatch_11.dll`, `nvinfer_vc_plugin_11.dll` : redistribuables mais **à ne pas livrer** (inutiles ; SEC-12 exclut les moteurs lean / version-compatible). Livrer uniquement ce qui est réellement chargé (`sm120` suffit pour la cible RTX 50) | Supplément 2 ; SLA 1.1 (iii) |
| Non redistribuable | `trtexec.exe` (outil de développement : usage interne), `lib/*.lib` (bibliothèques d'import, inutiles à l'exécution), roues `python/`, exemples non modifiés | SLA 1.2 (4e puce) ; 2.2 |
| Conditions de redistribution | (a) DLL **incorporées** dans Enhance, sous forme binaire, Enhance apportant une fonctionnalité substantielle propre ; (b) DLL **accessibles uniquement par Enhance** (sous-dossier de l'application, chargement par chemin absolu, jamais dans `PATH` ni System32) ; (c) CLUF d'Enhance **cohérent** avec le SLA : interdiction de rétro-ingénierie, de modification et de redistribution séparée des composants NVIDIA, protection de la propriété intellectuelle de NVIDIA ; (d) protéger la vie privée et la sécurité des utilisateurs (zéro réseau) | SLA 1.1 (iii), 1.2 |
| Interdits | Modifier, patcher, re-signer ou décompresser les DLL ; retirer les mentions de copyright ; contourner une protection ; présenter Enhance comme « soutenu / approuvé par NVIDIA » | SLA 2.1, 2.2, 2.3, 2.4 |
| Licence d'Enhance | Ne pas placer l'ensemble sous une licence copyleft qui imposerait de fournir le SDK en source ou de le rendre librement redistribuable. Publication **gratuite, code privé** (hypothèse PO n° 4) : compatible. Un éventuel passage en open source devra laisser les DLL NVIDIA hors du périmètre de la licence (composant tiers séparé) ; à revoir avec SEC-21 | SLA 2.5 |
| Moteurs (plans) générés au premier lancement | Construits **sur la machine de l'utilisateur**, par l'utilisateur, avec les DLL redistribuées : usage normal du runtime, pas une distribution de notre part → **OK**. NVIDIA conserve ses droits sur le SDK et ses dérivés (art. 3.1) : ne pas proposer d'exporter / partager les moteurs, et **ne pas livrer de moteurs pré-construits** dans l'installeur sans nouvel examen. Le modèle ONNX compilé reste soumis à sa propre licence (RIFE : SEC-03) | SLA 3.1 ; 1.1 (iii) |
| Publication gratuite | **Compatible** : aucune redevance ni restriction commerciale ; seules les obligations ci-dessus | SLA 1.1–1.2 |
| Autres obligations | Contrôle des exportations (pas de livraison vers un pays sous embargo américain) ; signaler à NVIDIA tout usage non conforme connu ; une résiliation n'affecte pas les copies déjà distribuées conformément | SLA 7 ; 1.2 ; 6.4 |

**Verdict : OK sous conditions** (usage local et redistribution des DLL listées ci-dessus). Confirmations du PO du 2026-10-21 : (1) en téléchargeant l'édition « Enterprise », il n'a accepté **aucune autre condition** que ce SLA, ce qui lève le doute sur le mot « Enterprise » ; (2) copie PDF du SLA archivée hors dépôt (voir ci-dessus). Les conditions restantes sont des **obligations de livraison** (ENH-030), pas des points ouverts : DLL non modifiées, chargées par Enhance seul, `Acknowledgements.txt` livré, CLUF d'Enhance (SEC-21) reprenant l'art. 1.2, aucun moteur pré-construit livré. En cas de doute futur sur la distribution : contact prévu au Supplément art. 3. Piste à étudier plus tard par le gpu-engineer (non bloquante) : **TensorRT for RTX**, sous sa propre licence à relire.

### 2.5 CUDA Toolkit 13.4.3 (runtime lié statiquement)

- Fichier lu : `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.4\EULA.txt` (installé sur la machine du PO, version 13.4.3 d'après `version.json`).
- Art. 1.1.1.3 et 2.2 : distribution des seules parties listées en **Attachment A** ; Attachment A (art. 2.6), composant « CUDA Runtime », Windows : `cudart.dll, cudart_static.lib, cudadevrt.lib` (et variantes nommées par version, ex. `cudart64_110.dll`).
- Conditions (art. 1.1.2) : fonctionnalités additionnelles substantielles ; parties distribuées accessibles uniquement par notre application ; conditions de distribution cohérentes avec l'EULA ; outils de développement (`nvcc`, etc.) à usage interne uniquement. Art. 2.1 : applications pour systèmes à GPU NVIDIA. Art. 1.2 : pas de licence open source sur le SDK (même clause que ci-dessus).
- `nvcuda.dll`, NVML : fournis par le pilote.
- **`cuda.lib` (ENH-040, lié dans `enh_rtxvideo_bridge.dll`)** : dans CUDA 13.4, `lib/x64/cuda.lib` n'est plus une simple bibliothèque d'import de `nvcuda.dll` mais un **chargeur statique** (8 objets `cuda_driverloader_*.obj`, appel `LoadLibraryExA("nvcuda.dll")`, constaté par `ar t` le 2026-10-24) : son code est incorporé au pont qui sera livré. `cuda.lib` **n'est pas listé** dans l'Attachment A pour Windows (art. 2.6 : seuls `cudart.dll`, `cudart_static.lib`, `cudadevrt.lib` pour le runtime ; le composant « NVIDIA CUDA Driver Libraries » ne cite que des fichiers Linux). Lecture stricte : redistribution non accordée ; lecture usuelle : bibliothèque de liaison indispensable à toute application qui utilise l'API pilote. Je ne suis pas juriste → **à confirmer par le PO** avant ENH-030, ou, mieux, **supprimer la question** : le pont charge `nvcuda.dll` lui-même (`LoadLibraryExW(..., LOAD_LIBRARY_SEARCH_SYSTEM32)` + `GetProcAddress` des ~11 fonctions utilisées) et ne lie plus `CUDA::cuda_driver` (SEC-35, recommandé).

**Verdict : OK** pour `cudart` lié statiquement (ADR-021/048) ; **à confirmer** pour `cuda.lib` intégré au pont RTX Video (ENH-040, voir ci-dessus ; non bloquant tant que rien n'est livré, à lever avant ENH-030).

---


## Sommaire (découpage du 2026-10-29)

- § 1-2 vue d'ensemble, SDK NVIDIA : [licences.md](licences.md)
- § 3-7 composants livrés, outils, verdicts, règles, livrable, points à confirmer : [licences-composants.md](licences-composants.md)
