> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../backlog.md).

## Seuils QA de référence (protocole `docs/qa/protocole.md` §7, à jour de la décision n° 14)

**Qualité** (jeu A, moyenne sur les clips à vérité terrain **et** minimum par clip), écart à Lanczos mesuré sur les mêmes images. Notation : `marge = 100 - VMAF Lanczos`.

| Préréglage | Delta VMAF 4K minimal | SSIM / PSNR | Garde-fou VMAF-NEG | Minimum par clip |
|---|---|---|---|---|
| **Qualité max** | >= **50 %** de la marge | jamais sous Lanczos sur aucun clip ; seuils chiffrés en part de marge **à fixer par le qa-tester** (protocole §7.1) | delta NEG >= 50 % de (100 - NEG Lanczos) **[H]** | aucun clip sous Lanczos + 25 % de la marge **[H]** |
| **Équilibré** | >= **35 %** | idem | >= 35 % **[H]** | aucun clip sous Lanczos + 15 % **[H]** |
| **Rapide** | >= **20 %** | idem | >= 20 % **[H]** | aucun clip sous Lanczos |

Exemples chiffrés (VMAF Lanczos du 2026-10-21, jeu A) : **BBB-PAN Kept 82,57** -> Qualité max >= 91,3 ; Équilibré >= 88,7 ; Rapide >= 86,1. **SYN-TEXT 63,74** -> >= 81,9 ; 76,4 ; 71,0. **TOS-FACE 92,81** -> >= 96,4 ; 95,3 ; 94,3. **BBB-FAST Kept 92,26** -> >= 96,1 ; 95,0 ; 93,8. **TOS-DARK 95,90** -> >= 97,9 ; 97,3 ; 96,7. Ces valeurs sont des **cibles non encore mesurées** : la première mesure d'Enhance (ENH-009) peut les contredire ; en cas d'écart, la QA remonte le chiffre au PO (jamais de relance jusqu'à PASS).

**SSIM / PSNR (décision n° 24, validés le 2026-10-24)** : gain de PSNR vs Lanczos >= **+3,0 dB** (Qualité max) / **+1,9 dB** (Équilibré) / **+1,0 dB** (Rapide) ; (1 - SSIM) réduit d'au moins **x 0,5 / 0,65 / 0,8** vs Lanczos (autrement dit, (1 - SSIM) <= 0,5 / 0,65 / 0,8 fois celui de Lanczos) ; VMAF-NEG : part de marge du préréglage **- 10 pts**. Réajustables après le spike ENH-053.

**Interpolation** (images `Generated`) : >= valeur du spike - 1 -> **BBB-FAST >= 90,5**, **BBB-PAN >= 89,9**, **SYN-TEXT >= 68,7** (RIFE mixte, 720p, interpolation seule, modèle VMAF par défaut : 91,5 / 90,9 / 69,7) ; 1er centile >= celui de la duplication ; Rapide : >= duplication + 1.

**Contre NVIDIA Broadcast 2.3** (jeu B, >= 2 clips) : VMAF >= Broadcast - 0,5 ; SSIM et PSNR sans tolérance. Rapport Broadcast **en cours de production par le qa-tester** (`docs/qa/resultats/2026-10-21_reference-broadcast.md`, à venir) : tant qu'il n'existe pas, ce seuil est « non vérifié », jamais PASS par défaut.

**Vitesse** (images de sortie / s, médiane de 5 passes, valeur soutenue) : budgets **Qualité max >= 30**, **Équilibré >= 60**, **Rapide >= 100**. Mesuré au spike (marge) : 36,5 HEVC / 51,5 AV1 ; 103-112 ; 219 (**la marge de Qualité max est faible : 1,2x en HEVC**). PASS / avertissement / FAIL : voir protocole §7.2.

**Ressources** : VRAM pic (pic - repos, in-process) **avertissement > 3 Gio** (mesuré 1,5-2,1 Gio), **FAIL > 9 216 Mio** (budget d'architecture) ; dérive VRAM sur 10 fichiers <= 50 Mio ; dérive mémoire système <= 100 Mio ; aucun plantage sur les cas limites (protocole §8). **Régression** : FAIL si > 5 % de perte (VMAF, SSIM, PSNR, i/s) ou > 5 % de VRAM en plus par rapport à la dernière mesure de `main`.
