> Extrait de l'ancien `docs/gpu/architecture-pipeline.md` (ménage docs du 2026-10-29), texte inchangé. Sommaire : [architecture-pipeline.md](architecture-pipeline.md).

## 4. Préréglages et réglages d'encodage [H]

Les préréglages sont des **données** (fichier versionné, ENH-018, géré par le developer) converties par une fonction pure de l'application en `PipelineParams` typé (§ 6.2). Valeurs initiales proposées, toutes **[H]** :

| | Qualité max | Équilibré (défaut) | Rapide |
|---|---|---|---|
| Super-résolution (niveau `NVSDK_NGX_VSR_QualityLevel`, 0 bicubique … 4 Ultra) | 4 Ultra | 4 Ultra (coût marginal : 3,0 ms contre 2,3 ms en 3 High **[M]**) | 2 Medium |
| Interpolation (< 50 i/s → 60) | RIFE, pleine qualité | RIFE, pleine qualité | dégradée (FRUC ou RIFE à flux demi-résolution) — seul préréglage autorisé à dégrader (décision PO n° 5) |
| NVENC préréglage / réglage | P7, HQ (UHQ si disponible) | P5, HQ | P3, HQ |
| Contrôle de débit | VBR, multipasse pleine résolution, anticipation 32 | VBR, multipasse quart de résolution, anticipation 16 | VBR, une passe, anticipation 0 |
| Débit cible 4K60 HEVC / AV1 (kbit/s) | 60 000 / 45 000 | 40 000 / 30 000 | 25 000 / 19 000 |
| Profondeur par défaut | 10 bits (Main10 / AV1 Main 10 bits) ; 8 bits proposé pour compatibilité | idem | idem |
| Images/s de sortie visées (720p30 → 4K60) | ≥ 30 | ≥ 60 | ≥ 100 |

Débits ajustés au prorata des pixels et de la cadence si la sortie n'est pas 3840×2160 à 60 i/s (ex. source ≥ 50 i/s non interpolée à 50 i/s, sortie 2880×2160).

---

## 5. Budget VRAM et vitesse par étage [H]

Contexte machine : 12 227 Mio au total, ~2 052 Mio pris au repos → ~10 175 Mio libres. Budget job : **≤ 9 216 Mio** (vision §5). Mesure = pic − repos (`cuMemGetInfo` + DXGI `QueryVideoMemoryInfo`, cf. `spike.md` R4).

### 5.1 VRAM par étage, chaîne nominale 720p30 → 4K60 (interpolation avant SR)

| Étage | Contenu | Budget **[H]** (Mio) | Critère de story |
|---|---|---|---|
| Contexte CUDA + modules + cuBLAS/cuDNN éventuels de TensorRT | | 400 | — |
| Décodage NVDEC | surfaces FFmpeg (~20 × 1,4–2,8 Mio en 720p) + état NVDEC ; jusqu'à 1080p 10 bits | 600 | ENH-008 ≤ 600 |
| Pool source (pivot FP16) | 8 × 720p RGBA16F (7 Mio) = 56 ; 1080p : 8 × 16 = 128 | 256 | — |
| Interpolation RIFE TensorRT 720p | poids + activations + espace de travail | 1 024 | ENH-012 |
| Super-résolution RTX Video SDK 720p → 4K | ressources internes du SDK + tampons d'E/S | 2 560 → **mesuré [M] : 188 (effet q3/q4) + 48 (tampons)** | ENH-009 ≤ 3 072 |
| Pool 4K vers NVENC | 4 × P010 4K (12,4 Mio) + 2 × pivot 4K (jusqu'à 66 Mio en FP16) | 256 | — |
| Encodeur NVENC 4K 10 bits | références, anticipation 32, B-frames, tampons de sortie | 1 536 → **mesuré [M] : Équilibré HEVC 869 / AV1 1 266 ; P7 HQ 1 265 / 1 458 ; UHQ 2 094 / 2 262** | ENH-010/011 ≤ 1 536 (UHQ dépasse : critère à revoir si UHQ est retenu) |
| Réserve : SDR→HDR10 (plus tard) | | 512 | ENH-032 |
| Marge de fragmentation / pics transitoires | | 512 | — |
| **Total** | | **7 656** | ≤ 9 216 → marge **1 560** |

**Mesuré en phase 2 [M]** (`spike-phase2.md` § P2.3) : chaîne Équilibré complète (FRUC 720p + VSR q3 + conversions + NVENC HEVC, hors décodage) = **1 293 Mio** de pic (+ ~140 Mio de contexte CUDA) ; en AV1 : 1 690 Mio. Le budget de 7 656 Mio est très large ; à réviser à la baisse quand NVDEC et RIFE seront mesurés.

Variante « interpolation après SR » : RIFE à 4K = 2 048–4 096 Mio **[H]** au lieu de 1 024, + pool 4K agrandi → **8 700–10 700 Mio**, hors budget dans le pire cas : argument supplémentaire pour l'ordre retenu. Mesuré avec FRUC **[M]** : variante « après SR » = 1 984 Mio (FRUC 4K = 735 Mio, 22,7 ms/image) et FRUC y répète toutes les images ; l'ordre « avant SR » est confirmé côté VRAM et vitesse (la qualité reste à mesurer).

Contrôle avant démarrage : somme des `vramEstimate()` des étages + marge comparée à la VRAM libre (`cuMemGetInfo`) ; si insuffisante → `PipelineErrorCode::OutOfVram` avec Mio requis / disponibles (ENH-014), pas de plantage. Une allocation qui échoue en cours de route (`CUDA_ERROR_OUT_OF_MEMORY`, `NV_ENC_ERR_OUT_OF_MEMORY`) donne la même erreur, le job finit proprement.

### 5.2 Vitesse par étage, RTX 5070 [H]

| Étage | Débit visé **[H]** | Moteur |
|---|---|---|
| Décodage NVDEC 720p H.264/HEVC | ≥ 600 i/s (critère ENH-008 : ≥ 300) | NVDEC (1 moteur sur la 5070, à vérifier) |
| Conversions couleur + coupure de plan 720p | ≥ 2 000 i/s | SM |
| RIFE 720p FP16 | 250–400 images générées/s | SM / Tensor cores |
| SR 720p → 4K | 70–120 i/s **[H]** → **mesuré [M] : q3 421 i/s (2,3 ms), q4 322 i/s (3,0 ms) ; 10 bits identique** | SM / Tensor cores |
| Conversion pivot → P010 4K | ≥ 1 000 i/s | SM |
| NVENC HEVC 4K60 10 bits | P5 ≈ 100–130 i/s, P7 ≈ 50–70 i/s **[H]** → **mesuré [M] : P3 265 ; Équilibré (P5 HQ, multipasse ¼, anticipation 16) 103 ; P7 HQ multipasse pleine 37 ; P7 UHQ 29** | NVENC (**1 seul moteur, confirmé** par `AppEncCuda -ec`) |
| NVENC AV1 4K60 10 bits | P5 ≈ 80–120 i/s, P7 ≈ 45–60 i/s **[H]** → **mesuré [M] : P3 178 ; Équilibré 105 ; P7 HQ 52 ; P7 UHQ 38** | NVENC |

Débit de bout en bout ≈ min(NVENC, 1 / (temps SM par image de sortie)), où le temps SM par image de sortie = `t_SR + (fraction générée) × t_RIFE + conversions`. Exemple **[H]** Équilibré 30→60 : 10 ms + 0,5 × 3 ms + 0,3 ms ≈ 11,8 ms → ~85 i/s côté SM, plafonné par NVENC P5 ~110 → **~85 i/s**. Si la SR coûte 16 ms, on tombe à ~57 i/s : objectif Équilibré manqué → décision PO n° 5 (qualité d'abord, vitesse affichée).

**Phase 2 [M]** : temps SM par image de sortie (FRUC avant SR, VSR q3–q4) ≈ 3,9–4,6 ms, soit 220–260 i/s côté SM ; **chaîne Équilibré mesurée à 104–118 i/s, plafonnée par l'unique NVENC**. Qualité max ≈ 35 (HEVC) / 50 (AV1) i/s ; Rapide ≈ 170–200 i/s. RIFE (non mesuré) dispose donc d'une marge SM d'environ 10 ms par image générée avant de devenir limitant en Équilibré. Détails : `spike-phase2.md` § P2.3–P2.4.

---
