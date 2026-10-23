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
| RTX Video SDK 1.1.0 (`nvngx_vsr.dll`, `nvngx_truehdr.dll`) | **OK sous conditions** (+ 2 points à confirmer) | Art. 1.c : redistribution autorisée « incorporé dans une application » ; DLL `rel` seulement ; mention NVIDIA ; zéro réseau à prouver (SEC-01) |
| Video Codec SDK 13.1.15 | **OK** | Seuls les en-têtes (MIT) et éventuellement du code d'exemple (MIT) sont utilisés ; NVENC/NVDEC fournis par le pilote ; rien à redistribuer |
| Optical Flow SDK 5.0.7 — FRUC (`NvOFFRUC.dll`) | **BLOQUÉ** pour la livraison ; OK en local (**simple repli** depuis le 2026-10-21, SEC-04) | Art. 1.c : seul le code source d'exemple est distribuable ; la DLL FRUC ne l'est pas |
| TensorRT 11.4.0.106 « Enterprise » | **OK sous conditions** (local et livraison ; confirmations PO du 2026-10-21) | SLA SDK NVIDIA + Supplément TensorRT art. 2 : DLL d'exécution redistribuables, intégrées à Enhance, non modifiées, CLUF cohérent, `Acknowledgements.txt` livré (§2.4) |
| CUDA runtime 13.4 (`cudart_static.lib`) | **OK** | EULA CUDA, Attachment A : `cudart_static.lib` distribuable |
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

**Verdict : OK** (liaison statique de `cudart`, ADR-021/048 ; rien d'autre de CUDA n'est livré).

---

## 3. Bibliothèques et composants livrés ou liés

| Composant | Version (à épingler) | Licence | Usage | Redistribuable | Obligations | Verdict |
|---|---|---|---|---|---|---|
| **FFmpeg — build BtbN `win64-lgpl-shared`** | 8.x, tag + SHA-256 dans `deps/ffmpeg.lock.json` (SEC-17) | **LGPL-3.0-or-later** en pratique : `ffprobe -buildconf` du 2026-10-23 (n8.1.3-18-g46d899d7a0, tag `autobuild-2026-10-20-13-08`) montre `--enable-version3`, ni `--enable-gpl` ni `--enable-nonfree` ; bibliothèques externes compilées dedans sous leurs propres licences (dav1d, libaom, libvpx, rav1e, SVT-AV1, zimg, libxml2, freetype, libass, opencore-amr (Apache-2.0, d'où `version3`)…) ; `--enable-vapoursynth` présent (démuxeur refusé par la liste blanche d'ENH-008). `deps/ffmpeg.lock.json` indique à tort `LGPL-2.1-or-later` (correction devops) | **local** (spike, développement) | Oui en théorie, mais **non retenu pour la livraison** (tags purgés : sources correspondantes non garanties ; surface d'attaque, SEC-06) | — | **OK (développement uniquement)** |
| **FFmpeg — port vcpkg** (livraison) | version de la baseline vcpkg | LGPL-2.1-or-later | **livré** (DLL `avformat`, `avcodec`, `avutil`, `swscale`, `swresample`) | Oui | DLL séparées et remplaçables ; texte LGPL-2.1 livré ; mention dans « À propos » ; **sources exactes + configuration** (port, overlay, triplet) mises à disposition ; CLUF d'Enhance qui autorise l'ingénierie inverse pour déboguer les modifications (LGPL-2.1 §6) ; `check-ffmpeg-license.ps1` : ni `--enable-gpl`, ni `--enable-nonfree`, ni `libx264/x265/fdk`. **Feature `nvcodec`** : vérifier qu'elle n'active ni `cuda-nvcc` ni `libnpp` (options *nonfree*) ; `ffnvcodec` (en-têtes MIT) et `cuvid/nvdec/nvenc` sont compatibles LGPL | **OK** sous ces conditions |
| `nv-codec-headers` (ffnvcodec) | celle du port FFmpeg | MIT | build (en-têtes) | Oui | Mention MIT | **OK** |
| **dav1d** (décodeur AV1 logiciel `libdav1d`, repli d'ENH-008 quand NVDEC ne gère pas le flux : AV1 12 bits, 4:2:2/4:4:4) | dev : lié statiquement dans `avcodec-62.dll` de BtbN ; livraison : port vcpkg `dav1d` via `ffmpeg[dav1d]`, version de la baseline | **BSD-2-Clause** (VideoLAN) — le zip BtbN ne fournit pas le texte : **à relire** dans `vcpkg_installed/*/share/dav1d/copyright` lors d'ENH-048 / ENH-030 | **livré** (dans `avcodec` ou en `dav1d.dll`) | Oui | Avis de copyright + texte BSD-2-Clause dans `THIRD-PARTY-LICENSES.txt` (clause 2 : reproduire l'avis dans la documentation livrée avec les binaires) ; compatible LGPL, aucune contrainte de liaison | **OK** (consigné le 2026-10-23, ENH-008) |
| **Qt 6** (qtbase, qtdeclarative, qtsvg) | ≥ 6.8, version de la baseline vcpkg | LGPL-3.0-only (aussi GPL / commercial) | **livré** (DLL) | Oui | Liaison **dynamique** uniquement ; DLL remplaçables par l'utilisateur ; textes LGPLv3 **et** GPLv3 livrés ; mention Qt + licence dans « À propos » ; mise à disposition des **sources de Qt** et de la **configuration de build** (triplet + overlay ports versionnés, ADR-024) ; pas de clause interdisant l'ingénierie inverse nécessaire au remplacement. Modules autorisés : Core, Gui, Qml, Quick, Quick Controls (Basic/Templates), Quick Effects, Quick Dialogs, Svg (+ Qt Test / Quick Test en tests). **Interdits** : `QtNetwork` (zéro réseau) et tout module GPL seul ou commercial (ex. Charts, Graphs/Data Visualization, Quick 3D, Virtual Keyboard, Lottie… — vérifier le fichier de licence de **chaque** module avant tout ajout) | **OK** |
| Tiers inclus dans Qt (FreeType, HarfBuzz, libpng, libjpeg-turbo, zlib, PCRE2, double-conversion, md4c…) | selon la baseline | FreeType : FTL (choisie) ; HarfBuzz : MIT (« Old MIT ») ; libpng : libpng-2.0 ; libjpeg-turbo : IJG + BSD-3 + zlib ; zlib : Zlib ; PCRE2 : BSD-3 ; double-conversion : BSD-3 ; md4c : MIT | livré (dans les DLL Qt ou en DLL vcpkg) | Oui | Textes dans `THIRD-PARTY-LICENSES.txt` ; **FTL** : mention « Portions of this software are copyright © The FreeType Project (www.freetype.org) » dans la documentation ; **IJG** : « this software is based in part on the work of the Independent JPEG Group » | **OK** (inventaire automatique par `scripts/licences.ps1` depuis `vcpkg_installed/*/share/*/copyright` et `qt_attribution.json`) |
| qtshadertools, qttools (moc, rcc, qsb, lupdate, lrelease, windeployqt) | idem | LGPL/GPL (outils) | **build** | Non concerné | Ne pas livrer `Qt6ShaderTools.dll` ni les outils | **OK** |
| tl::expected | baseline vcpkg | CC0-1.0 | livré (en-tête) | Oui | Aucune | **OK** |
| spdlog | baseline vcpkg, feature `std-format` | MIT | livré (statique) | Oui | Texte MIT dans `THIRD-PARTY-LICENSES.txt` | **OK** |
| fmt | seulement si `std-format` est abandonné | MIT | livré (statique, via spdlog) | Oui | Texte MIT | **OK** |
| nlohmann/json | baseline vcpkg | MIT | livré (en-tête) | Oui | Texte MIT | **OK** |
| CRT Visual C++ (`vcruntime140.dll`, `vcruntime140_1.dll`, `msvcp140.dll`, `msvcp140_1/_2/_atomic_wait/_codecvt_ids.dll`, `concrt140.dll`) | v145 (14.51.36231) | Licence Visual Studio 2026 Community — « Distributable Code » (`Licenses/1036/Redist.txt` de VS, qui renvoie à la liste en ligne `aka.ms/vs/18/redistribution`) | **livré app-local** (ADR-032) | Oui, fichiers **non modifiés**, copiés depuis `VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT` | Ne livrer que les DLL nécessaires ; ne pas utiliser les marques Microsoft ; pas de version Debug (`*d.dll` non redistribuables) ; mettre à jour le CRT avec VS (correctifs de sécurité) | **OK** |
| Polices : **Segoe UI Variable** | système Windows 11 | Licence Windows (non redistribuable) | utilisée, **non livrée** | **Non** | Chargée depuis le système uniquement ; ne jamais la copier dans le dépôt ou le ZIP ; repli si absente (Windows 10) | **OK** |
| Polices : **Inter** (si choisie par l'ui-designer) | version officielle épinglée + SHA-256 | SIL OFL 1.1 | livré (embarqué via `FontLoader`) | Oui | Livrer `OFL.txt` ; ne pas vendre la police seule ; vérifier dans le `OFL.txt` fourni l'absence de « Reserved Font Name » (sinon ne pas renommer une version modifiée) | **OK** |
| SF Pro / SF Symbols | — | Licence Apple, plateformes Apple uniquement | — | **Non** | Interdits (règle PO) | **BLOQUÉ** |
| Jeu d'icônes (à choisir par l'ui-designer) | — | voir §5 | livré | — | — | **À CONFIRMER** |
| Modèle d'interpolation **RIFE 4.x** — code (Practical-RIFE / ECCV2022-RIFE) | commit `bbfd2ea9` | MIT | référence pour **notre** script d'export Python : **local** ; aucun code Python livré | Oui | Mention MIT si du code est repris (y compris dans notre script d'export versionné) | **OK** |
| Modèle d'interpolation **RIFE 4.25** — **poids** d'origine (`flownet.pkl`, lien « Trained Model » du README de Practical-RIFE) | 4.25 ; SHA-256 à consigner au téléchargement (fait par le PO) | **MIT** selon le README de `hzwer/Practical-RIFE` au commit `bbfd2ea9` (2026-08-27), section « Trained Model » : *« The content of these links is under the same MIT license as this project. »* (citation relevée par le gpu-engineer, `docs/gpu/spike.md` §P3.1 ; **non relue par moi** : pas d'accès réseau) | entrée de l'export (local) ; l'ONNX dérivé est **livré** (ADR-043) | **Oui** (MIT) | Texte MIT + ligne de copyright du `LICENSE` de Practical-RIFE dans `THIRD-PARTY-LICENSES.txt` et « À propos » ; archiver hors dépôt (`third_party/models/rife/_licences/`) une copie du README et du `LICENSE` au commit `bbfd2ea9` + SHA-256 de `flownet.pkl` | **OK sous conditions** — à confirmer par le PO : (a) relire lui-même la phrase du README ; (b) données d'entraînement (Vimeo-90K, usage de recherche) : risque résiduel jugé faible pour une diffusion gratuite, que je ne peux pas trancher (je ne suis pas juriste) |
| **ONNX RIFE 4.25 exporté par Enhance** (export maison depuis `flownet.pkl`) | à produire ; SHA-256 dans un verrou versionné (ex. `deps/models.lock.json`) | MIT (poids) + notre code d'export | **livré** | **Oui** | Script d'export **écrit par nous** à partir du code MIT de Practical-RIFE, **sans reprendre le script d'export ni l'enveloppe « v2 » de vs-mlrt** (GPL-3.0) ; bourrage fait dans notre code C++/CUDA ou dans notre propre enveloppe ; `torch.load(..., weights_only=True)` (un `.pkl` peut exécuter du code à la désérialisation) ; SHA-256 vérifiée avant chargement par TensorRT | **OK** (une fois produit et consigné) |
| ONNX **vs-mlrt** `rife_v2/rife_v4.25.onnx` (release `external-models` d'AmusementClub/vs-mlrt, SHA-256 `65c57a5e…d75584`) | publié le 2024-09-19 | Dépôt **GPL-3.0**, aucune licence propre aux fichiers ; graphe « v2 » (bourrage interne) = travail de vs-mlrt ; aucune métadonnée de licence dans le fichier (vérifié : seul « pytorch 2.2.0 » comme producteur) | **local** (essai du spike, référence de comparaison QA) | **Non** pour nous (provenance GPL, statut du fichier incertain) | Jamais commité ni livré ; ne pas copier son graphe ni son script | **BLOQUÉ** pour la livraison ; **OK en local** (la GPL n'impose rien à un usage privé) |
| ONNX Runtime (option B, si un jour retenue) | — | MIT | livré | Oui | Texte MIT + notices tierces de l'archive | **OK** (non utilisé à ce jour) |

---

## 4. Outils de build, de test, de CI (jamais livrés)

| Outil | Licence | Usage | Verdict / remarque |
|---|---|---|---|
| vcpkg (outil + registre) | MIT | build | **OK** ; `VCPKG_DISABLE_METRICS=1` (télémétrie de l'outil désactivée) |
| CMake (de VS 2026), Ninja | BSD-3-Clause ; Apache-2.0 | build | **OK** |
| MSVC / Visual Studio 2026 Community | Licence VS Community | build | **OK** pour un développeur individuel ; à revoir si le projet devient l'œuvre d'une organisation |
| CUDA Toolkit 13.4 (`nvcc`, outils) | EULA CUDA | build | **OK** ; outils à usage interne (art. 1.1.2.4) |
| LLVM clang-format / clang-tidy | Apache-2.0 avec exception LLVM | build | **OK** |
| GoogleTest | BSD-3-Clause | tests | **OK** |
| Qt Test, Qt Quick Test | LGPLv3 / GPLv3 | tests | **OK** |
| OpenCppCoverage | GPL-3.0 | outil local, non lié | **OK** (outil séparé, aucune contamination) |
| FFmpeg « outil » GPL + libvmaf (QA, ex. gyan.dev « full » ou BtbN `gpl`) | GPL-3.0 ; libvmaf BSD-2-Clause-Patent | **local**, hors dépôt, jamais lié ni livré | **OK** (accord du PO requis pour le téléchargement, ADR-025) |
| Python 3.12, PyTorch, `onnx` (export RIFE : les roues **CPU** suffisent pour `torch.onnx.export` ; cu128 seulement pour exécuter PyTorch sur la RTX 5070, ex. comparaison numérique) | PSF-2.0 ; BSD-3-Clause ; Apache-2.0 | local (export ONNX RIFE, scripts QA) | **OK** ; rien de Python n'est livré ; venv hors dépôt, versions épinglées (`pip install --require-hashes`) ; téléchargement soumis à l'accord du PO |
| Inno Setup 6 (plus tard) | licence Inno Setup (permissive, usage commercial autorisé) | build de l'installeur | **OK** ; l'installeur produit n'est pas soumis à la licence de l'outil |
| gitleaks | MIT | CI / local | **OK** (binaire épinglé + SHA-256 ; l'action `gitleaks-action` exige une clé de licence pour les comptes d'organisation : utiliser le binaire) |
| cppcheck | GPL-3.0 | CI / local | **OK** (outil) |
| semgrep CLI | LGPL-2.1 | CI | **OK** avec nos règles locales seulement et `--metrics=off` (pas de `--config auto`, qui exige l'envoi de métriques) ; les règles du registre Semgrep ont leur propre licence : non utilisées |
| osv-scanner | Apache-2.0 | CI | **OK** |
| zizmor (audit des workflows) | MIT | CI | **OK** (optionnel) |
| Actions GitHub : `actions/checkout`, `actions/cache`, `actions/upload-artifact` | MIT | CI | **OK**, épinglées par SHA |
| `jurplel/install-qt-action` (+ aqtinstall) | MIT | CI (tests UI uniquement, Qt officiel jamais livré) | **OK**, épinglée par SHA |
| `ilammy/msvc-dev-cmd` (si utilisée) | MIT | CI | **OK**, épinglée par SHA |
| Runners GitHub | Conditions GitHub | CI | **OK** ; aucun runner auto-hébergé (ADR-029) |

---

## 4 bis. Verdicts sur les téléchargements demandés (devops phase B, qa) — 2026-10-21

| Élément demandé | Par | Licence | Usage | Verdict | Conditions |
|---|---|---|---|---|---|
| vcpkg (outil, clone au SHA épinglé) | devops | MIT | build | **OK** | `-disableMetrics` + `VCPKG_DISABLE_METRICS=1` ; baseline = SHA 40 hex (remplacer `TODO-VCPKG-BASELINE-SHA`) |
| Ports `gtest`, `spdlog` (`std-format`), `nlohmann-json`, `tl-expected` | devops | BSD-3 ; MIT ; MIT ; CC0-1.0 | tests ; livré (statique / en-têtes) | **OK** | Textes MIT dans `THIRD-PARTY-LICENSES.txt` |
| Qt 6.8+ via vcpkg (`qtbase` sans `network`, `qtdeclarative` sans `qml-network`, `qtsvg`, `qtshadertools`, `qttools`) | devops | LGPL-3.0 (+ tiers §3) ; outils LGPL/GPL | livré (DLL) / build (outils) | **OK** | §3 (Qt) ; conserver les archives de sources de `downloads/` (obligation LGPL) ; vérifier après build l'absence de `Qt6Network.dll` (contrôle C2) |
| Outils annexes téléchargés par vcpkg (CMake, Ninja, 7-Zip, NuGet, PowerShell, Perl, Python embarqué, NASM/YASM, paquets MSYS2 pour FFmpeg…) | devops | licences libres diverses (BSD, Apache, LGPL, GPL pour certains outils) | **build uniquement** | **OK** | Jamais copiés dans le livrable ; vcpkg vérifie leurs SHA-512 |
| FFmpeg BtbN `win64-lgpl-shared` 8.x | devops / gpu-engineer | LGPL-2.1+ | **développement** | **OK** | Tag daté + SHA-256 dans un fichier de verrou **hors `third_party/`** (SEC-17) ; vérifier `ffmpeg -buildconf` (ni `--enable-gpl` ni `--enable-nonfree`) ; **jamais livré** (SEC-06) |
| FFmpeg BtbN `win64-gpl` (mesure VMAF, qa) | qa | GPL-3.0 (+ libvmaf BSD-2-Clause-Patent) | **local**, hors dépôt, jamais lié ni livré | **OK sur le plan licence** | Accord du PO requis (ADR-025) ; dossier séparé (ex. `C:\Tools\ffmpeg`), jamais dans `third_party/` ni dans un chemin vu par le build de l'app |
| OpenCppCoverage (optionnel) | devops | GPL-3.0 | outil local | **OK** | Outil séparé, non lié |
| `actions/checkout`, `actions/cache` | devops | MIT | CI | **OK** | Épinglées par SHA de commit (remplacer `TODO-SHA`) |
| `jurplel/install-qt-action` (+ aqtinstall) | devops | MIT | CI (tests UI) | **OK** | SHA épinglé ; Qt officiel en CI uniquement, jamais livré |
| clang-format via `pip` (paquet PyPI `clang-format`) | devops | Apache-2.0 avec exception LLVM | CI / local | **OK** | Version exacte épinglée (`==x.y.z`), idéalement `--require-hashes` |
| Clips *Big Buck Bunny*, *Tears of Steel* (download.blender.org) | qa | CC BY 3.0 (Blender Foundation) | local, non redistribués | **OK** | Attribution dans `docs/qa/clips.md` (présente) ; SHA-256 consignée |
| *Crowd Run* SVT (media.xiph.org/video/derf) | qa | annoncé CC BY 4.0 **[à confirmer]** (non vérifiable sans réseau) | local, non redistribué | **OK pour un usage local de test** (risque faible) ; **À CONFIRMER** avant toute publication d'extrait | Relever la mention de licence sur la page au téléchargement |
| *Sol Levante*, *Meridian* (Netflix Open Content) | qa | CC BY 4.0 ou CC BY-NC-ND 4.0 selon l'œuvre **[à confirmer]** | local, non redistribué | **OK pour un usage local de test** (même NC-ND : rien n'est diffusé ni vendu) | **Aucune** publication d'extrait, de version 720p ou traitée (ND) ; relever la licence exacte de chaque œuvre |
| Hôtes `download.blender.org`, `media.xiph.org`, `download.opencontent.netflix.com` | qa | — | script local `fetch-clips.ps1` | **OK** | HTTPS, liste d'hôtes fermée, SHA-256 vérifiée, jamais lancé en CI ; concerne le poste de dev, pas l'application |

## 5. Règles pour les ajouts futurs

- **Licences admises sans discussion** pour du code ou des ressources livrés : MIT, BSD-2/3, ISC, Zlib, Apache-2.0, CC0, Unlicense, Boost (BSL-1.0), OFL-1.1 (polices), CC-BY-4.0 (icônes, avec attribution dans « À propos »).
- **Sous conditions** : LGPL (liaison dynamique seulement, obligations ci-dessus), MPL-2.0 (fichiers modifiés publiés).
- **Interdites dans le livrable** : GPL / AGPL (tout lien), licences « non commerciales » (CC-BY-NC) si une diffusion est envisagée, licences propriétaires sans droit de redistribution écrit, SF Pro / SF Symbols, toute police système copiée.
- **Icônes** (en attente de l'ui-designer) : candidats compatibles, à vérifier sur le fichier de licence de la version téléchargée — Fluent UI System Icons (MIT), Material Symbols (Apache-2.0), Phosphor (MIT), Lucide (ISC), Tabler (MIT). Un seul jeu (exigence design).
- Toute nouvelle dépendance : ligne ajoutée ici **avant** le merge (Definition of Done), avec texte de licence lu dans le paquet réellement téléchargé.

## 6. Ce que doit contenir le livrable (ENH-030)

`LICENSES\` : LGPL-2.1 (FFmpeg), LGPL-3.0 + GPL-3.0 (Qt), MIT/BSD/… des composants, OFL (si Inter), licence d'Enhance (SEC-21) ; `THIRD-PARTY-LICENSES.txt` généré (vcpkg `copyright`, `qt_attribution.json`, `doc/Acknowledgements.txt` de TensorRT livré tel quel avec les DLL `nvinfer*`) ; `NOTICE-SOURCES.txt` (où obtenir les sources exactes de Qt et FFmpeg + configuration de build, durée de l'offre) ; « À propos » : Qt, FFmpeg, mention NVIDIA RTX Video SDK (§2.1). DLL NVIDIA livrables : `rel/nvngx_vsr.dll` (§2.1) et, les DLL TensorRT listées au §2.4 (sous-dossier `nvidia\tensorrt\`, chargées par chemin absolu). Jamais `NvOFFRUC.dll` tant que §2.3 n'est pas levé. CLUF d'Enhance : clauses de protection NVIDIA (SLA TensorRT 1.2, 2.1–2.3).

## 7. Points à confirmer par le PO

1. ~~**TensorRT « Enterprise »**~~ : **levé le 2026-10-21** (aucune autre condition acceptée, PDF du SLA archivé) ; reste l'obligation de CLUF (SEC-21) au moment de ENH-030 (§2.4).
2. **Poids RIFE** (mis à jour le 2026-10-21) : les poids Practical-RIFE sont annoncés **MIT** par l'auteur (README, commit `bbfd2ea9`) → livrables via un **export ONNX maison** ; l'ONNX vs-mlrt (dépôt GPL-3.0) reste local. À confirmer par le PO : relecture de la phrase du README ; risque résiduel des données d'entraînement Vimeo-90K (§3).
3. **FRUC** : demander à NVIDIA l'autorisation de redistribuer `NvOFFRUC.dll`, ou accepter qu'une version publique n'ait pas cette brique (§2.3). Depuis le 2026-10-21 : RIFE (export maison, poids MIT) devient la brique d'interpolation livrable ; FRUC n'est plus qu'un **repli local** (SEC-04), non livré.
4. **RTX Video SDK** : notification préalable à NVIDIA (supplément art. 4) pour une publication gratuite ; forme de la mention de marque (art. 7.1).
5. **Licence d'Enhance** (CLUF) : gratuiciel propriétaire ou licence permissive, **pas GPL** ; clauses NVIDIA + clause LGPL d'ingénierie inverse (SEC-21).
6. **Brevets HEVC / AV1** : les licences NVIDIA laissent les licences de brevets codecs au développeur ; risque jugé faible pour une diffusion gratuite d'un encodeur matériel, mais je ne peux pas l'affirmer.
7. ~~**Trailer `Co-Authored-By`**~~ : tranché par le PO — aucune mention de l'assistant dans les messages ; appliqué par le hook `commit-msg` et le job `identity` (`security.yml`).
