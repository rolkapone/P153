# Enhance — Backlog produit

Version : v0.2 du 2026-10-21 (scrum-master). Statut : **validé par le PO le 2026-10-21**.
Les chiffres des critères `[GPU]` sont des **hypothèses [H]** à confirmer après les spikes ENH-001/002 et par `docs/qa/protocole.md`.
Clip de référence : `REF-A` (720p 30 i/s, issu d'un 4K réduit) défini par ENH-005 ; « sur la 5070 » = RTX 5070 12 Go du PO.

## Plan de release

| Indicateur | Valeur |
|---|---|
| Total points MVP | 126 |
| Points livrés | 0 |
| Vitesse moyenne des sprints | 15 pts (défaut, pas d'historique) |
| Sprints restants estimés avant le MVP | 9 (126 / 15 = 8,4 arrondi au-dessus) |
| Dernière mise à jour | 2026-10-21 (création) |

Répartition : Fondations 29 pts, Pipeline GPU 38 pts, File / préréglages / UI / transverse 59 pts.
Sprint 1 recommandé (16 pts, à ramener à 15 en retirant ENH-006 si besoin) : ENH-003 (CI/repo, 5) + ENH-001 (spike VSR, 5) + ENH-005 (protocole QA, 3) + ENH-006 (threat model, 3). Le spike d'interpolation ENH-002 et le squelette Qt ENH-004 en sprint 2.

## Décisions du PO (2026-10-21)

1. **Cadence** : on n'interpole jamais au-delà de 60 i/s ; une source >= 50 i/s n'est pas interpolée ; 24 i/s -> vrai 60 i/s interpolé (ENH-012). **Précision** : toute source strictement sous 50 i/s (y compris 31-49 i/s) est interpolée vers 60 i/s.
2. **Conteneur MVP** : MP4 ; bascule automatique en MKV, avec message, si une piste de sous-titres est incompatible MP4 (ENH-013, ENH-024).
3. **Clips de référence** : clips libres (Blender, Xiph) + 1 ou 2 extraits des archives du PO, jamais commités (ENH-005, ENH-029).
4. **Publication** : gratuite, code privé pour l'instant ; FFmpeg LGPL en liaison dynamique (ENH-006, ENH-030).
5. **Si la 5070 ne tient pas 60 i/s** : qualité d'abord, vitesse mesurée et affichée ; interpolation dégradée seulement dans « Rapide » (ENH-002, ENH-012, ENH-018, ENH-020).
6. **Quitter pendant un traitement** : seule exception à « pas de confirmation ». La croix de la fenêtre réduit l'app dans la barre des tâches sans arrêter le traitement ; Ctrl+Q ouvre une petite feuille « Quitter et abandonner le traitement en cours ? » (ENH-021).
7. **Toolchain** : Visual Studio 2026 (MSVC v145) remplace VS 2022 ; le CUDA Toolkit (13.x <= 13.4, compatible pilote 617.42) doit accepter ce compilateur hôte (ENH-001, ENH-003).
8. **Pause** : la VRAM reste réservée (~7 Gio) pendant une pause pour une reprise instantanée (MVP) ; libérer la carte pendant une pause viendra avec la reprise après plantage (ENH-021, ENH-038).
9. **Sous-titres** : SRT/WebVTT convertis en `mov_text` pour rester en MP4 (texte conservé intégralement) ; bascule MKV seulement pour ASS/PGS/VobSub, pièces jointes ou audio non MP4. Précise la décision n°2 (ENH-013).
10. **Préparation des modèles d'interpolation** (construction du moteur TensorRT, ~1-3 min [H]) : au premier traitement, avec un message affiché à l'utilisateur, pas à l'installation ; mis en cache ensuite (ENH-012).

**À transmettre UX** (ux-designer) :
- Texte du message P-08 pour `SourceUnreadable` (fichier verrouillé ou sans droits d'accès) : à rédiger, ton humain, avec action possible (ENH-021/ENH-013 côté erreurs de lecture).
- Libellé de l'étape « préparation des modèles » (première utilisation, ~1-3 min, une seule fois) affiché dans la carte de job et le journal (ENH-012).

---

# MVP

## Epic A — Fondations techniques

### ENH-001 — Spike RTX Video SDK : super-résolution 720p -> 4K  [GPU] [Must | 5 pts]
En tant qu'équipe, nous voulons mesurer la super-résolution du RTX Video SDK sur la RTX 5070 afin de confirmer la faisabilité et fixer les budgets.
Critères d'acceptation :
- [ ] Étant donné le SDK posé par le PO dans `third_party/`, quand on lance le programme du spike sur `REF-A`, alors une vidéo 4K est produite et lisible.
- [ ] (GPU) sur `REF-A` : débit mesuré en images/s, VRAM pic en Mio, VMAF/SSIM/PSNR vs Lanczos consignés dans `docs/gpu/spike-vsr.md`.
- [ ] Les modes de qualité du SDK sont listés avec leur coût (i/s, VRAM).
- [ ] Les prérequis (pilote 617.42, CUDA 13.x <= 13.4 acceptant MSVC v145 / Visual Studio 2026, `sm_120`) sont documentés (décision PO n°7).
- [ ] Une décision go / no-go et les budgets proposés sont inscrits dans `docs/architecture.md` (ADR).
Notes : dépendance au téléchargement du SDK par le PO ; licence vérifiée par secdevops avant tout commit.

### ENH-002 — Spike interpolation : RIFE vs NVIDIA Optical Flow FRUC  [GPU] [Must | 5 pts]
En tant qu'équipe, nous voulons comparer RIFE (TensorRT/CUDA) et le FRUC de NVIDIA Optical Flow afin de choisir la brique de génération d'images.
Critères d'acceptation :
- [ ] Étant donné `REF-A` (30 -> 60 i/s), quand on exécute chaque candidat, alors on obtient i/s, VRAM pic et VMAF des images générées (sur un clip 60 i/s dont on retire une image sur deux) dans `docs/gpu/spike-interpolation.md`.
- [ ] Les licences de chaque candidat (code et poids du modèle) sont inscrites dans le registre (`docs/security/licences.md`).
- [ ] L'impact combiné super-résolution + interpolation sur la 5070 est estimé (budget i/s et VRAM de bout en bout).
- [ ] Un ADR choisit la brique, ou propose de dégrader l'interpolation dans le préréglage « Rapide » uniquement (décision PO n°5 : « Qualité max » et « Équilibré » ne sacrifient jamais la qualité).
Notes : dépend d'ENH-001 pour le budget combiné ; résout le risque R1.

### ENH-003 — Setup repo, build CMake/MSVC, CI et tests GPU locaux  [Must | 5 pts]
En tant que développeur, je veux un build reproductible et une CI afin de livrer chaque story proprement.
Critères d'acceptation :
- [ ] Étant donné un clone propre, quand on lance `scripts/build.ps1`, alors le projet compile avec MSVC v145 (Visual Studio 2026) et vcpkg en mode manifeste, sans erreur ni avertissement.
- [ ] Le workflow GitHub Actions exécute build, `clang-format`, `clang-tidy` et tests unitaires sans GPU, et passe au vert.
- [ ] `scripts/test-gpu.ps1` lance les tests de `tests/gpu/` en local et retourne un code non nul en cas d'échec.
- [ ] `git config user.email` est l'adresse noreply ; aucun fichier de la liste d'exclusion n'est suivi.
- [ ] Le CUDA Toolkit retenu (13.x <= 13.4, compatible pilote 617.42) accepte MSVC v145 comme compilateur hôte : un `.cu` minimal compile avec `nvcc` sans `-allow-unsupported-compiler` (décision PO n°7).
Notes : aucun push sans accord du PO ; décision vcpkg consignée en ADR ; ADR toolchain VS 2026 / CUDA (décision PO n°7).

### ENH-004 — Squelette de l'application Qt Quick + ADR du front  [UI] [Must | 5 pts]
En tant que PO, je veux une fenêtre Enhance qui s'ouvre, branchée sur le C++ afin de valider la stack.
Critères d'acceptation :
- [ ] Un ADR compare Qt Quick à au moins 2 alternatives (WinUI 3, WebView2) et est validé par le PO.
- [ ] Étant donné l'exécutable, quand on le lance, alors une fenêtre vide « Enhance » s'affiche en moins de 2 s, thème sombre, texte en français.
- [ ] Un `Q_PROPERTY` C++ est affiché dans le QML ; une action QML appelle le C++ ; aucune logique métier dans le QML.
- [ ] Un test unitaire du modèle C++ s'exécute sans GPU en CI.
- [ ] Rendu net à 100 % et 200 % de mise à l'échelle Windows.
Notes : Qt en liaison dynamique (LGPL), licence consignée.

### ENH-005 — Protocole qualité et clips de référence  [Must | 3 pts]
En tant que qa-tester, je veux un protocole de mesure afin que chaque seuil soit vérifiable.
Critères d'acceptation :
- [ ] `docs/qa/protocole.md` décrit la méthode (4K -> 720p -> traitement -> comparaison), les modèles VMAF, les formules PSNR/SSIM, la mesure d'i/s et de VRAM (`nvidia-smi` ou NVML).
- [ ] `docs/qa/clips.md` liste au moins 4 clips libres (Blender, Xiph : dessin animé, nature, visage, jeu rétro) avec source et licence ; un script les récupère, aucun n'est commité.
- [ ] 1 ou 2 extraits des archives du PO s'ajoutent aux clips libres (décision PO n°3) : jamais commités, décrits sans chemin personnel dans `docs/qa/clips.md`.
- [ ] Un script `tests/quality/` produit un tableau VMAF/SSIM/PSNR pour Lanczos, sans Enhance, en moins de 5 min par clip.
- [ ] La méthode de comparaison avec Broadcast 2.3 est décrite.

### ENH-006 — Threat model et registre des licences  [Must | 3 pts]
En tant que secdevops, je veux un modèle de menaces et un registre des licences afin de sécuriser la publication.
Critères d'acceptation :
- [ ] `docs/security/threat-model.md` couvre fichiers malformés, DLL tierces, chemins de fichiers, absence de réseau.
- [ ] `docs/security/licences.md` liste chaque dépendance (SDK NVIDIA, FFmpeg, Qt, police, modèles) avec licence, redistribuable (oui / non / à vérifier) et conditions.
- [ ] Une méthode de vérification « zéro réseau » est décrite (voir ENH-028).
- [ ] Hypothèse de publication consignée (décision PO n°4) : gratuite, code privé ; FFmpeg LGPL uniquement, en liaison dynamique (aucun composant GPL ni `--enable-nonfree`) ; toute licence incompatible avec ce cadre est marquée « non ».

### ENH-007 — Design system et tokens  [UI] [Must | 3 pts]
En tant que PO, je veux un design system de niveau « Apple » afin que tous les écrans soient cohérents.
Critères d'acceptation :
- [ ] Tokens (couleurs sombre et clair, typographie, espacements, rayons, ombres, durées 150-350 ms) fournis en fichier QML importable.
- [ ] Police libre validée par secdevops (Segoe UI Variable ou Inter OFL).
- [ ] Une maquette HTML animée du composant « carte de fichier » et du bouton principal est validée par le PO.
- [ ] Contraste >= 4,5:1 sur le texte, dans les deux thèmes.

## Epic B — Pipeline vidéo GPU

### ENH-008 — Démuxage FFmpeg et décodage NVDEC  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux ouvrir n'importe quel format vidéo afin de ne jamais convertir au préalable.
Critères d'acceptation :
- [ ] Étant donné MP4, MKV, AVI, MOV, WebM, MPEG-TS contenant H.264, HEVC, VP9, AV1, MPEG-2, quand on décode, alors toutes les images arrivent en mémoire GPU avec un nombre d'images égal à celui annoncé par `ffprobe`.
- [ ] (GPU) décodage 720p >= 300 images/s ; VRAM <= 600 Mio.
- [ ] Si NVDEC ne supporte pas le codec, repli sur décodage logiciel FFmpeg avec message au journal.
- [ ] Les ressources C (FFmpeg, CUDA) sont encapsulées en classes RAII ; aucune fuite sur 10 fichiers.
Notes : l'interface du décodeur est abstraite pour tests sans GPU.

### ENH-009 — Super-résolution RTX Video SDK dans le pipeline  [GPU] [Must | 8 pts]
En tant qu'utilisateur, je veux agrandir en 4K avec la meilleure qualité afin d'obtenir une image nette.
Critères d'acceptation :
- [ ] Étant donné `REF-A`, quand on agrandit en 3840x2160, alors la sortie fait exactement 3840x2160.
- [ ] (GPU) sur `REF-A`, préréglage Qualité max : VMAF >= Lanczos + 5, SSIM >= Lanczos + 0,02, PSNR >= Lanczos + 1,5 dB **[H]**.
- [ ] (GPU) >= 60 images/s sur la 5070 pour l'étape seule, VRAM <= 3 Gio **[H]**.
- [ ] Une source déjà en 4K ou plus est copiée sans agrandissement (avec message).
- [ ] Le SDK est derrière une interface `IUpscaler` ; un faux upscaler permet les tests en CI.
Notes : dépend d'ENH-001, ENH-008.

### ENH-010 — Encodage NVENC HEVC 8 et 10 bits  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux sortir en HEVC jusqu'à 10 bits afin de ménager les dégradés et la taille.
Critères d'acceptation :
- [ ] Étant donné une suite d'images 4K, quand on encode en HEVC Main10, alors `ffprobe` indique `hevc`, `yuv420p10le`, 3840x2160.
- [ ] (GPU) >= 90 images/s en 4K ; VRAM encodeur <= 1,5 Gio **[H]**.
- [ ] Débit cible par préréglage configurable (kbit/s) ; le débit réel est à +/- 15 % de la cible.
- [ ] Le fichier est lisible dans un lecteur standard (VLC et Films et TV de Windows).

### ENH-011 — Encodage NVENC AV1  [GPU] [Must | 3 pts]
En tant qu'utilisateur, je veux choisir AV1 afin d'avoir des fichiers plus petits à qualité égale.
Critères d'acceptation :
- [ ] Étant donné les mêmes images que ENH-010, quand on choisit AV1 10 bits, alors `ffprobe` indique `av1`, 10 bits, 3840x2160.
- [ ] (GPU) >= 60 images/s en 4K **[H]** ; VRAM encodeur <= 1,5 Gio **[H]**.
- [ ] À débit égal, VMAF AV1 >= VMAF HEVC - 0,5 sur `REF-A`.
- [ ] Si le codec n'est pas disponible, l'option est grisée avec une explication.

### ENH-012 — Interpolation d'images vers 60 images/s  [GPU] [Must | 8 pts]
En tant qu'utilisateur, je veux des vidéos fluides à 60 i/s afin de profiter d'un écran 4K récent.
Critères d'acceptation :
- [ ] Étant donné une source 24, 25, 30 i/s, quand on traite, alors la sortie est à 60 i/s constants et la durée est inchangée (+/- 1 image).
- [ ] Source strictement < 50 i/s, y compris 31 à 49 i/s (ex. 40 i/s) : interpolée vers 60 i/s constants (décision PO n°1).
- [ ] Source >= 50 i/s : aucune interpolation, cadence conservée, jamais au-delà de 60 i/s (message au journal) (décision PO n°1).
- [ ] Source 24 i/s : vrai 60 i/s interpolé (pas de pulldown 3:2), durée inchangée.
- [ ] Si le spike montre que la 5070 ne tient pas 60 i/s de bout en bout : « Qualité max » et « Équilibré » gardent la qualité (plus lent) ; seul « Rapide » dégrade l'interpolation (décision PO n°5).
- [ ] Changements de plan détectés : pas d'image interpolée à travers la coupure.
- [ ] (GPU) pipeline complet 720p -> 4K60 : >= 60 images/s de sortie en Équilibré, VRAM total <= 9 Gio **[H]**.
- [ ] (GPU) VMAF des images générées >= valeur du spike ENH-002 - 1.
- [ ] Préparation des modèles (construction du moteur TensorRT, ~1-3 min [H]) : lancée au premier traitement qui interpole, jamais à l'installation ; message/étape visible pour l'utilisateur pendant la préparation ; moteur mis en cache, traitement suivant sans préparation (décision PO n°10).
- [ ] Interface `IInterpolator`, faux interpolateur pour la CI.
Notes : dépend d'ENH-002 ; brique choisie par ADR.

### ENH-013 — Conservation de l'audio, des sous-titres et des métadonnées  [GPU] [Must | 3 pts]
En tant qu'utilisateur, je veux garder mes pistes audio et sous-titres afin de ne rien perdre.
Critères d'acceptation :
- [ ] Étant donné un MKV avec 2 pistes audio et 2 sous-titres texte, quand on traite, alors la sortie contient les mêmes pistes (audio copié sans réencodage, flux identiques en durée à +/- 40 ms).
- [ ] Conteneur de sortie MP4 par défaut ; si une piste de sous-titres est incompatible MP4 (ex. PGS), bascule automatique en MKV avec message avant le traitement listant les pistes concernées ; aucune piste n'est perdue (décision PO n°2).
- [ ] SRT/WebVTT convertis en `mov_text` : sortie `.mp4`, texte conservé intégralement (décision PO n°9). Bascule MKV seulement pour ASS/PGS/VobSub, pièces jointes ou audio non MP4.
- [ ] Test : MKV avec sous-titres PGS (ou ASS/VobSub) -> sortie `.mkv` et message ; MKV avec sous-titres SRT/WebVTT -> sortie `.mp4` avec texte identique après conversion.
- [ ] Les métadonnées de titre et de langue sont conservées.
- [ ] Synchro audio / vidéo vérifiée sur un clip de 10 min : décalage < 40 ms.

### ENH-014 — Budget VRAM et absence de fuite  [GPU] [Must | 3 pts]
En tant que PO, je veux que l'app tienne dans 12 Go afin de continuer à utiliser mon PC.
Critères d'acceptation :
- [ ] `docs/architecture.md` fixe le budget par étape (décodage, upscaling, interpolation, encodage) dont la somme <= 9 Gio **[H]**.
- [ ] (GPU) test sur 10 fichiers à la suite : dérive de VRAM <= 50 Mio entre le premier et le dernier.
- [ ] Si la VRAM libre est insuffisante au démarrage d'un job, il passe en erreur lisible plutôt que de planter.

### ENH-015 — Robustesse face aux fichiers malformés  [GPU] [Must | 3 pts]
En tant qu'utilisateur, je veux qu'un fichier corrompu n'arrête pas la file afin de laisser tourner pendant la nuit.
Critères d'acceptation :
- [ ] Étant donné 10 fichiers tronqués ou corrompus (corpus de test), quand on les traite, alors aucun plantage : chaque job finit en état « Échec » avec un message en français.
- [ ] Un fichier corrompu au milieu d'une file ne bloque pas les suivants.
- [ ] Aucune exception ne traverse les frontières de l'API (`std::expected` ou codes d'erreur).
Notes : fuzzing léger avec secdevops.

## Epic C — File d'attente et préréglages

### ENH-016 — Modèle de la file d'attente  [Must | 5 pts]
En tant que développeur, je veux un modèle de file côté C++ afin que l'UI ne contienne aucune logique.
Critères d'acceptation :
- [ ] États d'un job : En attente, En cours, En pause, Terminé, Échec, Annulé ; transitions testées en unitaire (>= 90 % des chemins).
- [ ] Ajout, suppression, déplacement d'un job testés sans GPU.
- [ ] Exposé en `QAbstractListModel` ; notifications fines par signaux.
- [ ] Probe du fichier (durée, résolution, i/s, codec) via FFmpeg en moins de 1 s par fichier.

### ENH-017 — Glisser-déposer et ajout à la file  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux glisser des vidéos dans la fenêtre afin de les ajouter à la file.
Critères d'acceptation :
- [ ] Étant donné la fenêtre vide, quand on survole avec des fichiers, alors une zone de dépôt réagit en moins de 100 ms (animation 150-250 ms).
- [ ] Le dépôt de 20 fichiers les ajoute en moins de 2 s, avec miniature, nom, durée et résolution.
- [ ] Les fichiers non vidéo sont refusés avec un message bienveillant, sans bloquer les autres.
- [ ] État vide illustré avec un bouton « Choisir des fichiers » (alternative clavier).
- [ ] Un fichier ajouté deux fois est signalé (sans boîte de confirmation).

### ENH-018 — Préréglages et choix du codec  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux choisir « Qualité max », « Équilibré » ou « Rapide » afin de ne pas toucher aux réglages techniques.
Critères d'acceptation :
- [ ] Trois préréglages, « Équilibré » sélectionné par défaut ; choix HEVC ou AV1 par un contrôle segmenté.
- [ ] Chaque préréglage a une description d'une ligne et une estimation de durée affichée pour le fichier sélectionné.
- [ ] Seul « Rapide » peut dégrader l'interpolation pour gagner en vitesse ; sa description l'indique clairement (décision PO n°5).
- [ ] Le préréglage choisi s'applique à toute la file et est mémorisé entre deux lancements.
- [ ] Définition des préréglages (modes SDK, débit, 10 bits, interpolation) dans un fichier de données versionné, pas en dur dans le QML.
- [ ] Navigation entièrement au clavier.

### ENH-019 — Exécution séquentielle de la file  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux que les fichiers se traitent l'un après l'autre afin de lancer et partir.
Critères d'acceptation :
- [ ] Étant donné 3 fichiers, quand on clique sur « Démarrer », alors ils sont traités dans l'ordre, un seul à la fois.
- [ ] Un échec ou une annulation n'empêche pas le suivant.
- [ ] (GPU) file de 10 fichiers de 30 s : tous terminés, dérive VRAM <= 50 Mio.
- [ ] À la fin, notification discrète (non bloquante) avec le récapitulatif.
Notes : dépend d'ENH-009/010/012/016.

### ENH-020 — Progression : images/s et temps restant  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux voir où en est le traitement afin d'estimer l'attente.
Critères d'acceptation :
- [ ] Barre de progression par fichier et globale, images/s et temps restant mis à jour au plus toutes les 500 ms.
- [ ] La vitesse réelle mesurée (images/s) est toujours affichée, y compris quand elle est inférieure au temps réel (décision PO n°5).
- [ ] Temps restant avec une erreur <= 15 % après les 10 premières secondes.
- [ ] (GPU) animation de l'interface >= 60 images/s pendant un traitement 4K (mesure QA).
- [ ] Les valeurs sont annoncées aux lecteurs d'écran à intervalle raisonnable (<= 1 fois / 5 s).

### ENH-021 — Pause et annulation  [UI] [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux mettre en pause ou annuler afin de reprendre la main sur mon PC.
Critères d'acceptation :
- [ ] Pause (décision PO n°8) : la VRAM reste réservée (~7 Gio) pendant la pause ; reprise instantanée (< 1 s) **[H]** ; le calcul GPU s'arrête en moins de 2 s ; reprise sans perte d'images (nombre d'images final identique à un traitement sans pause).
- [ ] Annulation : arrêt en moins de 3 s, VRAM libérée, fichier partiel supprimé, annulation possible via « Annuler » dans les 5 s (pas de boîte de confirmation).
- [ ] Raccourcis : Espace (pause), Échap (annuler le job en cours, avec annulation possible).
- [ ] (GPU) 20 cycles pause/reprise : aucune fuite VRAM.
- [ ] Quitter pendant un traitement (décision PO n°6) : la croix de la fenêtre réduit l'app dans la barre des tâches, le traitement continue (images/s inchangées) ; Ctrl+Q ouvre la feuille « Quitter et abandonner le traitement en cours ? » (Annuler / Quitter) ; confirmer = annulation propre (VRAM libérée, fichier partiel supprimé) ; sans traitement en cours, Ctrl+Q quitte sans feuille. C'est la seule confirmation de l'app.

### ENH-022 — Réordonnancement de la file  [UI] [Must | 3 pts]
En tant qu'utilisateur, je veux déplacer des fichiers dans la file afin de choisir ce qui passe en premier.
Critères d'acceptation :
- [ ] Glisser une carte la déplace avec animation (ressort, 150-350 ms) ; alternative clavier (Alt + flèches).
- [ ] Le job en cours ne peut pas être déplacé ; le reste le peut.
- [ ] L'ordre est conservé après redémarrage de l'app (file persistée).

### ENH-023 — Ouvrir le fichier produit  [UI] [Must | 2 pts]
En tant qu'utilisateur, je veux ouvrir le résultat afin de le vérifier tout de suite.
Critères d'acceptation :
- [ ] Sur un job Terminé : boutons « Lire » (application par défaut) et « Afficher dans le dossier » (Explorateur avec le fichier sélectionné).
- [ ] Fichier déplacé ou supprimé : message clair, bouton désactivé.

### ENH-024 — Dossier de sortie et nommage  [Must | 2 pts]
En tant qu'utilisateur, je veux savoir où sont mes fichiers afin de les retrouver.
Critères d'acceptation :
- [ ] Par défaut, sortie à côté de la source avec le suffixe `_Enhance_4K60` ; ne jamais écraser un fichier existant (suffixe numérique).
- [ ] Choix d'un dossier de sortie unique dans les réglages.
- [ ] Chemins avec accents, espaces, > 260 caractères et lecteurs réseau : cas de test unitaires.
- [ ] Espace disque insuffisant : avertissement avant le démarrage (estimation de taille).

### ENH-025 — Journalisation locale  [Must | 2 pts]
En tant que développeur, je veux des journaux locaux afin de diagnostiquer sans réseau.
Critères d'acceptation :
- [ ] Journal fichier tournant (5 x 2 Mio) dans `%LOCALAPPDATA%\Enhance\logs`, niveaux Info/Avertissement/Erreur.
- [ ] Aucun chemin de fichier utilisateur en clair par défaut sauf le nom ; aucune donnée personnelle.
- [ ] Bouton « Afficher les journaux » dans les réglages.

### ENH-026 — Thèmes sombre et clair, accessibilité  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux un thème sombre soigné et un thème clair complet afin de travailler confortablement.
Critères d'acceptation :
- [ ] Sombre par défaut ; option « Suivre le système » ; changement animé en moins de 300 ms sans scintillement.
- [ ] Contraste >= 4,5:1 (texte) et 3:1 (composants) dans les deux thèmes.
- [ ] Mise à l'échelle Windows 100, 125, 150, 200 % : aucun texte tronqué, rendu net.
- [ ] Tout est accessible au clavier avec un focus visible ; noms accessibles pour Narrateur.
- [ ] Option « Réduire les animations » respectée si activée dans Windows.

### ENH-027 — Interface en français  [UI] [Must | 2 pts]
En tant qu'utilisateur francophone, je veux une interface en français afin de tout comprendre.
Critères d'acceptation :
- [ ] Toutes les chaînes via `qsTr` / fichiers de traduction ; aucun texte en dur en C++ ou QML.
- [ ] Dates, durées et tailles formatées à la française (ex. « 1,2 Gio », « 3 min 05 s »).
- [ ] Revue de relecture des textes (micro-textes humains, tutoiement ou vouvoiement fixé par l'ux-designer).
Notes : architecture prête pour une autre langue plus tard.

### ENH-028 — Garde-fou « zéro réseau »  [Must | 2 pts]
En tant que PO, je veux la preuve que l'app n'émet rien afin de garantir la confidentialité.
Critères d'acceptation :
- [ ] Un test automatisé échoue si le binaire importe des API réseau (analyse des imports DLL : `ws2_32`, `winhttp`, `wininet`, `QtNetwork`) hors liste blanche documentée.
- [ ] Test manuel : application lancée avec un pare-feu bloquant tout, fonctionnement identique, 0 tentative de connexion journalisée.
- [ ] Qt déployé sans module Network.

### ENH-029 — Banc d'essai qualité automatisé vs Lanczos et Broadcast  [Must | 5 pts]
En tant que qa-tester, je veux un rapport automatique afin de suivre les seuils à chaque story.
Critères d'acceptation :
- [ ] Un script produit en une commande un tableau (VMAF, SSIM, PSNR, images/s, VRAM pic) pour chaque clip et préréglage.
- [ ] Comparaison avec Lanczos systématique, avec Broadcast 2.3 sur au moins 2 clips (processus manuel documenté) ; clips libres + extraits des archives du PO, jamais commités.
- [ ] Alerte si régression > 5 % par rapport à la dernière mesure de `main`.
- [ ] Résultats consignés dans `docs/qa/` ; les clips ne sont pas commités.

### ENH-030 — Version portable pour la démo  [Must | 3 pts]
En tant que PO, je veux un dossier à lancer afin de tester sur ma machine sans environnement de développement.
Critères d'acceptation :
- [ ] `scripts/package.ps1` produit un dossier / zip avec l'exécutable et ses DLL, démarrant sur une machine sans Visual Studio.
- [ ] FFmpeg est fourni en DLL (LGPL, liaison dynamique) ; le fichier de licences indique comment obtenir les sources LGPL (décision PO n°4).
- [ ] Les DLL redistribuées sont toutes dans `docs/security/licences.md` avec une mention « redistribuable : oui ».
- [ ] Un fichier de licences tierces est inclus.

### ENH-031 — Détection des prérequis (carte, pilote)  [UI] [Must | 3 pts]
En tant qu'utilisateur, je veux un message clair si ma machine n'est pas compatible afin de comprendre pourquoi.
Critères d'acceptation :
- [ ] Au lancement, détection de la GPU (RTX 50 attendue) et de la version du pilote ; message en français avec la version minimale requise.
- [ ] GPU non compatible : l'app s'ouvre en lecture seule (aucun traitement) avec explication, sans plantage.
- [ ] Tests unitaires de la logique de détection avec une interface simulée (CI sans GPU).

---

# Plus tard (vision long terme, visible pour l'architecture)

Contraintes d'architecture à tenir dès le MVP : étapes du pipeline en modules chaînés (ajout d'une étape HDR sans réécriture), description d'un job sérialisable (reprise, dossiers), préréglages en données, conteneur de sortie paramétrable, décodeur capable de fournir des images 10 bits.

### ENH-032 — SDR -> HDR10 (RTX Video SDK)  [GPU] [UI] [Should | 8 pts]
Sortie HEVC/AV1 10 bits HDR10 avec métadonnées ; case à cocher dans les préréglages ; mesure de VRAM et i/s ; validation visuelle sur un écran HDR.

### ENH-033 — Comparaison avant / après côte à côte  [UI] [GPU] [Should | 8 pts]
Curseur de comparaison sur une image ou quelques secondes ; 60 i/s d'animation ; lecture synchronisée.

### ENH-034 — Préréglages personnalisés  [UI] [Could | 5 pts]
Créer, dupliquer, renommer, supprimer ; stockage local versionné.

### ENH-035 — Options avancées : débit, résolution, cadence cibles, MP4/MKV  [UI] [GPU] [Could | 8 pts]
Panneau masqué par défaut, valeurs validées côté C++.

### ENH-036 — Aperçu rapide sur quelques secondes  [UI] [GPU] [Should | 5 pts]
Traitement de 5 s choisies en moins de 15 s **[H]** avant de lancer le fichier complet.

### ENH-037 — Traitement de dossiers entiers  [UI] [Could | 5 pts]
Dépôt d'un dossier, parcours récursif, filtre des vidéos, structure conservée.

### ENH-038 — Reprise après plantage  [GPU] [Should | 8 pts]
Points de reprise tous les N segments ; reprise propre au redémarrage ; test par arrêt brutal.
- [ ] Libération de la VRAM pendant une pause (décision PO n°8) : une pause peut libérer la carte (état repris depuis un point de reprise) ; test : VRAM libérée pendant la pause, reprise sans perte d'images.

### ENH-039 — Installeur Windows propre  [Should | 5 pts]
Installeur signé ou non (décision PO), désinstallation propre, aucun accès réseau ; devops + secdevops.

Total « Plus tard » : 52 pts.
