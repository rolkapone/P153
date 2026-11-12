# Sprint 2 - seuils QA (une passe pour tout le sprint)

Planning du 2026-10-29, qa-tester. Aucune mesure faite. Sources : [protocole](protocole.md) §4, §5, §7 ; notes [ENH-041](../gpu/ENH-041.md), [ENH-042](../gpu/ENH-042.md), [ENH-053](../gpu/ENH-053.md) ; baseline `main` : [ENH-047](resultats/2026-10-24_ENH-047.md). Pas de story `[UI]` : aucune capture.

## Conditions communes (rétro Sprint 1)
- Avant toute mesure officielle : `nvidia-smi` (utilisation GPU < 5 %, temp. < 50 C, VRAM de fond notée), navigateur / Docker / iCUE fermés, pilote 617.42 noté (sinon noté et baseline rejouée).
- A/B dans la **même session** : baseline `main` (decode BBB-FAST 4100, BBB-PAN 2870 i/s ; VSR q4 300 / 315 i/s) rejouée avant la branche. Écart 5-15 % entre deux passes = « non concluant » (on rejoue, on ne masque pas) ; (max-min)/médiane > 3 % = instable, signalé.
- Médiane de 5 passes après chauffe, 20 s de pause. Régression > 5 % vs `main` = FAIL.

## ENH-041 (RENFORCÉE) - preuve zéro réseau et durcissement
Mesures : `scripts/test-no-network-dynamic.ps1`, M1 pare-feu, M2 audit WFP, M3 capture (nomenclature de la fiche). Cibles `enhance-bench.exe`, `enh_gpu_rtxvideo_tests.exe`.
| Contrôle | PASS | FAIL |
|---|---|---|
| Processus enfant (pont, NGX) | 0 | >= 1 |
| Paquets / connexions TCP-UDP du processus (M1-M3) | 0 | >= 1 |
| Requêtes DNS du processus (journal DNS-Client) | 0 | >= 1 |
| Événements WFP 5156-5159 des processus visés | 0 | >= 1 (NVIDIA : consigné R-8, rapport secdevops) |
| Fichier nouveau dans magasin NGX / dossier de données | 0 | >= 1 |
| Téléchargement de DLL par NGX | jamais | escalade immédiate PO |
| `ChildProcessBlocker` / Job Object indisponible | refus (`NgxUpdaterNotBlocked`), 0 appel `rvb_runtime_create` | init qui continue |
| M1 : durée d'init vs sans règle | écart <= 10 % (pas d'attente 15-30 s), sorties identiques (SHA-256) | autre |
| Exports du pont | 34 exacts, 0 import `nvcuda`/`cudart` | autre |
| Surcoût d'init (`--stage vsr-image`) | <= +300 ms | > +300 ms |
| VRAM / débit | +0 Mio ; VSR q4 >= 285 i/s (-5 % de 300) en A/B | en dessous |
| 10 cycles init | dérive VRAM <= 50 Mio | > 50 Mio |
CRL/OCSP observé : exception à documenter, décision PO (non PASS par défaut). Les tests CI (faux bloqueur, `NgxStoreGuard`) sont vérifiés verts, pas remesurés.

## ENH-042 (STANDARD) - conversions couleur
Clips : image NV12 et P010 décodées par NVDEC sur BBB-FAST et SYN-TEXT (jeu B `in720`, texte fin) pour le réel ; pour l'aller-retour, **mire synthétique générée en RGB** (donc dans le gamut), 1280x720 et 3840x2160 :
- mire A « chrominance lisse » : dégradés linéaires H et V de R, G, B (pas de saut de chrominance entre pixels voisins), plus gris 0-255 en rampe ; sert au mode `Bilinear` ;
- mire B « arêtes franches » (damier 16 px couleurs primaires/secondaires) : seulement en `Nearest` (inverse exact).
| Contrôle | PASS | FAIL |
|---|---|---|
| Sortie GPU vs `CpuColorConverter` (4 noyaux, 2 modes, 601/709, 2 plages, tailles impaires, 1x1, 8192x4320) | identique bit à bit (0 octet différent) | 1 octet |
| Aller-retour `Nearest` (mires A et B) | écart max <= 1/255 (8 bits), <= 2/1023 (10 bits) | au-delà |
| Aller-retour `Bilinear` (mire A seule) | écart max <= 1 niveau 8 bits / 2 niveaux 10 bits ; moyennes Y/U/V <= 1/1023 | au-delà |
| Temps 4K ABGR10->P010 et ARGB8->NV12 (événements CUDA, médiane 100 passes après 10 de chauffe) | <= 0,3 ms/image | > 0,3 ms |
| VRAM allouée par les noyaux | 0 Mio ; pic test 4K <= 100 Mio | > 0 / > 100 Mio |
| 10 fichiers enchaînés | dérive <= 50 Mio | > 50 Mio |
| BT.2020 / PQ / HLG | `UnsupportedFormat` | conversion silencieuse |
| Tailles limites | refus > 8192x4320, 0 plantage | plantage |
Tolérance « ±1 niveau » = valeurs ci-dessus ; l'interprétation du mot « mire » (lisse en `Bilinear`, franche seulement en `Nearest`) confirmée ici.

## ENH-053 (RENFORCÉE) - spike SR
Clips : jeu B `tests/clips/in720/` (BBB-FAST, BBB-PAN, SYN-TEXT ; TOS exclus), vérité terrain `ref4k/`. FFmpeg 8.1.3, `vmaf_4k_v0.6.1` et NEG, notés. Mesures : VMAF, NEG, SSIM Y, PSNR Y par clip et moyenne, 1er centile VMAF, i/s SR seule, VRAM pic-repos.
Qualité vs Lanczos jeu B (parts §7.1, décision n° 24), pour un moteur à retenir pour le préréglage :
| Préréglage | Part VMAF | NEG | (1-SSIM) | ΔPSNR | Min par clip |
|---|---|---|---|---|---|
| Qualité max | >= 50 % | >= 40 % | <= 0,5 x Lanczos | >= +3,0 dB | >= Lanczos +2 VMAF |
| Équilibré | >= 35 % | >= 25 % | <= 0,65 x | >= +1,9 dB | >= +1 VMAF |
| Rapide | >= 20 % | >= 10 % | <= 0,80 x | >= +1,0 dB | >= Lanczos |
Qualité max, en plus, vs Broadcast jeu B : VMAF et NEG >= Broadcast - 0,5 ; SSIM et PSNR >= Broadcast, par clip (Broadcast non fourni = « non vérifié », jamais PASS). Égalité (écart < 1 VMAF) : VSR préférée.
Vitesse SR seule (720p->4K, médiane de 5) : >= 1,5 x budget = **45 / 90 / 150 i/s** (Qualité max / Équilibré / Rapide). VRAM SR seule <= 3 072 Mio (au-dessus : avertissement ; > 9 216 Mio FAIL). Moteur retenu = le plus rapide qui passe tout.
Aucun moteur ne passe un préréglage = FAIL signalé, meilleur compromis consigné, remonté au PO (aucun seuil assoupli en silence). Contrôle visuel texte SYN-TEXT et bords BBB-FAST (halos). Non-régression : VSR q4 >= 285 i/s en A/B.
Contrôle ONNX/TRT (ml-engineer, vérifié) : ONNX vs PyTorch >= 60 dB, FP16 vs FP32 >= 50 dB.

## Ce que le PO doit faire
1. Avant mesures GPU (042, 053) : fermer navigateur, Docker, iCUE, Teams ; machine au calme, aucune tâche lourde ni mise à jour en cours, alimentation « Performances élevées ».
2. ENH-041 : lancer **lui-même** le script `scripts/test-no-network-dynamic.ps1` dans **PowerShell en administrateur** (M1, M2, M3 ; une commande, aucun agent ne simule de frappe), laisser la machine sans autre usage réseau pendant ~5 min, réseau actif pour M3.
3. ENH-053 : produire ses sorties NVIDIA Broadcast 2.3 sur le jeu B si elles manquent (BBB-FAST, BBB-PAN, SYN-TEXT, largeur 3840) ; accorder les téléchargements (poids Real-ESRGAN, Python 3.12, torch CPU) après validation secdevops.
