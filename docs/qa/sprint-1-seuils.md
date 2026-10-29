# Sprint 1 — Seuils QA (ENH-004, ENH-008, ENH-040, ENH-047)

Auteur : qa-tester, 2026-10-21. Statut : **proposé** ; arguments alignés sur `docs/gpu/ENH-047.md` (échantillonnage VRAM 10 ms, image BMP, nom de fichier seul dans le JSON). Base : `docs/qa/protocole.md` (§4 vitesse, §5 VRAM, §7.3 ressources), `docs/gpu/spike-phase2.md` (P2.3 ; `spike-phase3.md` P3.6), `docs/design/ui/design-system-mouvement-composants.md` §12. Rien n'est exécuté à la rédaction (pas de build disponible). Arguments de `enhance-bench` : ceux de `docs/gpu/ENH-047.md` (`--stage decode|vsr-image`, `--in`, `--out`, `--report`, `--repeat`, `--warmup`, `--max-frames`, `--decoder auto|nvdec|software`, `--q`, `--bits`, `--size`, `--iterations`, `--sample-ms`).

## 1. Conditions de mesure (communes)

- RTX 5070, pilote 617.42 noté dans le rapport, carte au repos (< 5 % d'utilisation, < 50 °C), mode « Performances élevées », navigateur / Docker / iCUE fermés (~2 Gio de VRAM déjà pris : toujours mesurer en **différence** pic − repos).
- Protocole §4 : 1 chauffe non comptée + 5 répétitions, **médiane**, min-max ; (max − min) / médiane > 3 % = mesure **instable**, signalée, jamais relancée jusqu'à un beau chiffre.
- FFmpeg de mesure : `C:\Tools\ffmpeg\bin` (8.1.3 GPL, libvmaf), jamais lié à l'app. Version dans chaque rapport.
- Régression : première mesure = baseline (`docs/qa/resultats/baseline-main.json`) ; ensuite −5 % i/s ou +5 % VRAM = FAIL.
- Constat sur les clips existants : `in720/TOS-DARK.mkv` et `TOS-FACE.mkv` sont en 1280×570 (bandes noires retirées) : **non valides** pour « 720p → 4K » ; ne servent qu'à la vitesse de décodage.

## 2. Clips

| Usage | Fichier (dans `tests/clips/`) | Détail |
|---|---|---|
| Décodage H.264 720p (jeu A, FFV1 pour la source, H.264 pour le débit réel) | `in720/BBB-FAST.mp4`, `in720/BBB-PAN.mp4` | H.264 8 bits 720p (jeu B) : cas nominal NVDEC, 1 000 i/s |
| Décodage sans perte | `in720/BBB-FAST.mkv`, `in720/SYN-TEXT.mkv` | FFV1 : **non** passé à NVDEC (non pris en charge) → doit déclencher le **repli logiciel** + message au journal (critère ENH-008) |
| VSR image (qualité) | `in720/BBB-FAST.mkv`, `in720/BBB-PAN.mkv`, `in720/SYN-TEXT.mkv` (720p, jeu A) ; vérité `ref4k/<id>.mkv` | Image n° 150 de chacun (BBB = « jeu A » animation ; SYN-TEXT = texte fin) |
| HEVC 10 bits (généré, ENH-008) | `gen/BBB-FAST_hevc10.mkv` | voir §5 |
| AV1 (généré, ENH-008) | `gen/BBB-FAST_av1.mkv` | voir §5 |
| Boucle de 10 fichiers | 10 copies/extraits de `in720/BBB-FAST.mp4` + `BBB-PAN.mp4` en alternance (+ le HEVC10 et l'AV1 générés) | Fichiers réels distincts, pas le même fichier ouvert 10 fois |
| Cas limites (ENH-008 T5) | `gen/` : corrompu, tronqué, 0 octet, nom Unicode, chemin > 260 | générés localement, §5 |

`tests/clips/gen/` est déjà couvert par l'ignore de `tests/clips/`. Aucun téléchargement.

## 3. Seuils PASS / FAIL par story

| Story | Mesure | PASS | FAIL | Méthode (§4) |
|---|---|---|---|---|
| **ENH-008** décodage | i/s NVDEC 720p H.264 (médiane de 5, `--stage decode`) | ≥ 1 000 | < 1 000 (avertissement entre 800 et 1 000, = −20 %) | §4.1 |
| | Images décodées = `ffprobe -count_frames` | égalité exacte, tous formats du corpus | un écart | §4.2 |
| | VRAM décodage (pic − repos, in-process) | ≤ 600 Mio | > 600 Mio | §4.1 |
| | Écart `enhance-bench` / `nvidia-smi` (pic global − repos) | < 10 % **ou** vérification croisée DXGI `QueryVideoMemoryInfo` < 10 % (si l'écart tient à la granularité 200 ms ou au contexte du pilote, à expliquer) | ≥ 10 % non expliqué | §4.1 |
| | Boucle de 10 fichiers : dérive VRAM (fichier 10 − fichier 1, au repos 2 s après fermeture) | ≤ 50 Mio **et** pente non positive régulière ; mémoire privée ≤ 100 Mio ; handles/threads : 0 | sinon | §4.3 |
| | Repli logiciel (FFV1, format non pris en charge par NVDEC) | décode, i/s noté, message français/journal présent, 0 crash | crash ou silence | §4.4 |
| | Cas limites (protocole §8.1, 2, 4, 7 : corrompu, tronqué, 0 octet, Unicode, > 260) | pas de plantage, erreur claire ou décodage partiel annoncé, VRAM libérée (± 50 Mio) | un plantage = bloquant | §4.4 |
| **ENH-040** pont | VSR 720p→4K q4, 10 bits, i/s (médiane) | ≥ 300 (spike : 335) ; avertissement 240-300 | < 240 | §4.5 |
| | VSR 720p→4K q4, **8 bits**, i/s (médiane) — décision PO n° 20 (2026-10-24) | **avertissement** (240-300, à consigner, pas bloquant) ; baseline 2026-10-24 : 289,9 i/s (instable) | < 240 | §4.5 |
| | VRAM de l'effet (pic − avant création) | ≤ 300 Mio (spike : 188) | > 300 Mio | §4.5 |
| | Sortie q4 ≠ bicubique (q0), sur image réelle | ≥ 30 % d'octets différents (spike : 59 %) | égale | §4.5 |
| | Imports : aucun `cudart*.dll` ; DLL `rel` seulement | `dumpbin /dependents` propre | sinon | §4.6 |
| | 10 cycles création / destruction | dérive ≤ 50 Mio, 0 handle | sinon | §4.5 |
| **ENH-047** banc | JSON conforme (`frames_in`, `frames_out`, `wall_s`, `fps_out`, `stages[]`, `vram_peak_mib`, `vram_rest_mib`, `process_private_mib`, `exit_code`) | tous les champs, valeurs cohérentes (`fps_out ≈ frames_out/wall_s` à 1 %) | champ absent ou incohérent | §4.7 |
| | Répétabilité de `fps_out` | (max − min)/médiane ≤ 3 % | > 3 % : signalé instable | §4.1 |
| | Écart banc / `nvidia-smi` | < 10 % (voir ENH-008) | ≥ 10 % | §4.1 |
| | `Measure-Speed.ps1` branché sur le banc | s'exécute de bout en bout en mode Speed et Leak | échec | §4.1 |
| | Code de sortie | 0 en succès ; ≠ 0 et message clair sur fichier illisible | plantage | §4.7 |
| **ENH-009 / spike qualité SR** (déplacé depuis ENH-040, décision PO n° 20) VSR image, qualité (mode `vsr-image`) ; mesurer aussi sur le **jeu B compressé** (H.264) | Sur image 720p → 4K d'un clip jeu A, vs `ref4k` : VMAF, SSIM Y, PSNR Y de VSR q4, comparés à Lanczos | **Part Équilibré** (protocole §7.1) sur **chaque** image : part VMAF ≥ 35 % de la marge (VMAF ≥ L + 0,35 × (100 − L)) ; (1 − SSIM) ≤ 0,65 × celui de Lanczos ; ΔPSNR ≥ +1,87 dB ; part VMAF-NEG ≥ 25 % (garde-fou) | une métrique sous Lanczos = FAIL ; sous la part = FAIL (cible Qualité max 50 % : information) | §4.8 |
| **ENH-004** UI | Voir `docs/qa/checklist-ui.md` ; seuils chiffrés ci-dessous | | | |

Seuils UI chiffrés (design-system §12) : 60 i/s au repos et pendant une charge GPU, 99e centile du temps d'image ≤ 16,7 ms (33 ms toléré sur < 0,1 % des images) ; temps GPU de l'interface ≤ 1 ms moyen / ≤ 2 ms pire ; **0 image rendue** au repos pointeur immobile ; **écart d'i/s du pipeline entre verre complet et opaque ≤ 2 %** (médiane de 5, mêmes conditions, vitesse instable = non concluant) ; démarrage à froid < 2 s ; aucun `Qt6Network.dll`.

Note sur le seuil « 2 % » : il est plus étroit que la tolérance de répétabilité (3 %). La mesure ne conclut que si (max − min)/médiane de chacune des deux séries est ≤ 2 % ; sinon verdict « non concluant », pas PASS. Pour gagner en sensibilité : 7 répétitions, alternées (verre, opaque, verre, opaque…).

## 4. Méthode et commandes

Variables : `$FF = C:\Tools\ffmpeg\bin`, `$B = <build>\enhance-bench.exe`, `$C = F:\P153\tests\clips`, `$R = F:\P153\docs\qa\resultats\sprint-1`.

### 4.1 Décodage : vitesse, VRAM, croisement

```powershell
nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits -lms 200 | Out-File $R\smi.csv   # en tâche de fond
1..6 | % { & $B --stage decode --in $C\in720\BBB-FAST.mp4 --warmup 30 --sample-ms 10 --report $R\dec_$_.json }              # warmup 30 images exclues ; 6 passes, la 1re non comptée
```
Retenir médiane de `fps_out` (runs 2-6), `vram_stage_mib` (= pic − repos DXGI, déjà calculé par le banc ; `vram_device_used_peak_mib` sert au recoupement avec `nvidia-smi`). Croisement : (pic `smi.csv` − repos `smi.csv`) contre la VRAM du banc, écart relatif < 10 %. Ou via le mode Speed/Leak du script : `pwsh tests/quality/Measure-Speed.ps1 -Bench $B -InputFiles <fichier> -OutJson $R\speed.json -OutDir $env:TEMP\enh-out -Mode Speed -Repeat 5` (à adapter une fois `--stage` pris en charge).

### 4.2 Nombre d'images (par fichier du corpus)

```powershell
& "$FF\ffprobe" -v error -count_frames -select_streams v:0 -show_entries stream=nb_read_frames -of csv=p=0 <fichier>
```
comparé à `frames_in` du JSON du banc (égalité exacte ; pour un fichier tronqué : ≤ ffprobe et l'erreur est annoncée).

### 4.3 Boucle de 10 fichiers

`Measure-Speed.ps1 -Mode Leak -InputFiles <10 fichiers distincts>` ; relevé VRAM au repos 2 s après chaque fermeture ; dérive = point 10 − point 1 ; pente par régression linéaire des 10 points ; mémoire privée début/fin et handles/threads (`Get-Process`). Puis annulation en cours de décodage + 1 fichier : retour au repos de la boucle ± 50 Mio.

### 4.4 Repli logiciel et cas limites

Corpus généré localement (§5), même boucle ; contrôle du journal (message du repli) et du code de sortie ; VRAM avant/après.

### 4.5 VSR : vitesse, VRAM, cycles

Banc `--stage vsr-image` n'est qu'une image : pour i/s et VRAM, utiliser le test `tests/gpu` d'ENH-040 (`--frames 300 --q 4 --bits 10`, spike_bench équivalent) ou, à défaut, le mode vitesse du banc ; 5 répétitions, médiane. 10 cycles création/destruction : VRAM résiduelle après chaque cycle (même méthode que 4.3).

### 4.6 Imports

```powershell
dumpbin /dependents <build>\rtxvideo_bridge.dll        # attendu : aucun cudart*.dll ; nvcuda.dll seulement côté CUDA
```

### 4.7 JSON et code de sortie

Lire le JSON, vérifier les champs et la cohérence ; `$LASTEXITCODE` sur fichier inexistant, 0 octet, corrompu.

### 4.8 Qualité d'une image VSR (une image 720p → 4K)

1. Extraire l'image n de l'entrée et de la vérité : `& "$FF\ffmpeg" -v error -i $C\in720\BBB-FAST.mkv -vf "select=eq(n\,150)" -frames:v 1 -pix_fmt bgr24 $env:TEMP\in.bmp` ; de même `$C\ref4k\BBB-FAST.mkv` → `ref.bmp` (3840×2160 ; BBB-FAST est 60 i/s : même numéro d'image que la source 720p60 ; vérifier au PSNR que n et la vérité sont alignées : contrôle de décalage ±1 comme protocole §3.3).
2. Sortie VSR : `& $B --stage vsr-image --in in.bmp --out vsr.bmp --q 4 --bits 10 --iterations 300 --report $R\vsr.json` (le même appel donne i/s et `vram_stage_mib` VSR, avec décodage du BMP 24 bits hors chronométrage ; BMP 24 bits, sortie 8 bits). Lanczos : `ffmpeg -i in.bmp -vf scale=3840:2160:flags=lanczos lan.bmp`.
3. Mesure, une image, deux fois (VSR et Lanczos), les trois fichiers en `format=yuv420p10le` sans conversion de matrice (le BMP est RGB : conversion unique BT.709 limitée appliquée **identiquement** aux trois) :
```powershell
& "$FF\ffmpeg" -v error -i vsr.bmp -i ref.bmp -lavfi "[0:v]format=yuv420p10le[d];[1:v]format=yuv420p10le[r];[d][r]libvmaf=model=version=vmaf_4k_v0.6.1\:name=std|version=vmaf_4k_v0.6.1neg\:name=neg:feature=name=psnr|name=float_ssim:log_fmt=json:log_path=vsr.json" -f null -
```
(syntaxe de `Measure-Quality.ps1` à réutiliser en priorité). Une seule image rend VMAF bruité : mesurer **3 images** par clip (n = 100, 150, 200) et juger sur la **moyenne** et sur le pire. Les trois clips (BBB-FAST, BBB-PAN, SYN-TEXT) sont jugés ; moyenne des parts ≥ 35 % **et** aucun sous Lanczos. Conversion BMP → mesure : consigner que le chiffre n'inclut pas de perte d'encodage (≠ chiffre du protocole §2.3 point 5) ; il vise la super-résolution seule.
4. Attention : Broadcast n'est pas comparé ici (sortie vidéo, pas image).

## 5. Génération locale des clips supplémentaires (ENH-008, sans téléchargement)

À placer dans `tests/quality/New-DecodeCorpus.ps1` (à écrire avec le banc), sortie `tests/clips/gen/` :

```powershell
$FF='C:\Tools\ffmpeg\bin'; $src="$C\in720\BBB-FAST.mp4"; $o="$C\gen"; mkdir $o -Force
& "$FF\ffmpeg" -y -i $src -t 10 -c:v libx265 -pix_fmt yuv420p10le -crf 20 -tag:v hvc1 "$o\BBB-FAST_hevc10.mkv"
& "$FF\ffmpeg" -y -i $src -t 10 -c:v libsvtav1 -crf 30 -pix_fmt yuv420p10le "$o\BBB-FAST_av1.mkv"      # sinon libaom-av1 -cpu-used 8, sinon av1_nvenc
& "$FF\ffmpeg" -y -i $src -t 10 -c:v libvpx-vp9 -crf 30 -b:v 0 "$o\BBB-FAST_vp9.webm"
& "$FF\ffmpeg" -y -i $src -t 10 -c:v mpeg2video -q:v 3 "$o\BBB-FAST_mpeg2.ts"
Copy-Item "$o\BBB-FAST_hevc10.mkv" "$o\té 日本語 & espace.mkv"                                         # Unicode, & et espaces
# tronqué (60 %), corrompu (octets écrasés au milieu), 0 octet : via .NET (FileStream) sur BBB-FAST.mp4
```
Un encodeur absent du FFmpeg de mesure est noté « non généré » (pas de téléchargement) et le critère correspondant « non vérifié ». La sortie de ENH-008 doit gérer AV1 et HEVC 10 bits sur la 5070 (NVDEC : oui) ; VP9 et MPEG-2 : décodage NVDEC ou repli logiciel, les deux acceptés si journalisé.

## 6. Ce que le PO doit faire lui-même

- Narrateur (Win+Ctrl+Entrée) : écouter la fenêtre et le contrôle de démonstration (noms et rôles lus) ; je ne peux pas valider ce qu'il dit à voix haute.
- Changer la mise à l'échelle Windows (100 / 125 / 150 / 200 %, déconnexion éventuelle) et le thème sombre/clair ; activer/désactiver Paramètres > Accessibilité > Effets de transparence, Effets d'animation, Thèmes de contraste.
- Juger à l'œil le verre (scintillement au redimensionnement, netteté du texte, lisibilité sur fond blanc/noir/saturé) : les chiffres de la checklist ne remplacent pas l'œil.
- Fermer navigateur / Docker / iCUE avant les mesures de vitesse (sinon l'instabilité > 3 % sera signalée), brancher la machine, ne pas toucher au PC pendant les passes de 5 répétitions.
- Fournir les DLL/SDK nécessaires déjà posés dans `third_party/` si absents du build (FFmpeg LGPL partagé, RTX Video SDK).

Note 2026-10-24 (décision PO n° 20) : ENH-040 est jugée sur le périmètre du pont ; le critère de qualité VSR image est porté par ENH-009 et le spike « qualité de la super-résolution » (VSR niveaux 1-4 sur jeu B compressé, Real-ESRGAN TensorRT). Premier constat (jeu A, q4, 3 images/clip) : part VMAF moyenne +13,3 % (BBB-FAST −9,7, BBB-PAN +15,6, SYN-TEXT +34,0), SSIM pire que Lanczos sur les 9 images : voir `docs/qa/resultats/2026-10-24_ENH-040.md`.
