# Enhance — Architecture du pipeline vidéo GPU (proposition)

Statut : v0.2 du 2026-10-21 (gpu-engineer) — **contrat pipeline ↔ application aligné** avec `docs/architecture/index.md` (ex-`docs/architecture.md` v0.1 ; § 12 = `docs/architecture/interface-pipeline.md` ; ADR-002, ADR-003, ADR-010) ; résumé et tableau des ADR dans `docs/architecture.md` § 13.
Entrées : `docs/vision.md` v0.2, `docs/backlog.md` v0.2 (décisions du PO n° 1 à 7, aujourd'hui `docs/decisions.md`), `docs/gpu/spike.md` (phase 1), `docs/design/ux/parcours.md` (P-08).

Historique : v0.1 (proposition, ADR numérotés P01–P11) → v0.2 : ADR renumérotés **ADR-040 à ADR-051** (plage réservée au pipeline), contrat public réécrit (§ 6), écarts avec l'application tranchés (§ 6.0).

**Avertissement (mis à jour après la phase 2 du spike, 2026-10-21).** Les SDK NVIDIA sont posés (RTX Video SDK 1.1.0, Video Codec SDK 13.1.15, Optical Flow SDK 5.0.7, TensorRT 11.4) et leurs en-têtes ont été lus : les points levés citent le fichier dans `docs/gpu/spike-phase2.md` § P2.2, et les chiffres mesurés sont marqués **[M]** (§ P2.3, entrées synthétiques). Restent **« à vérifier »** : FFmpeg (absent), NVDEC, TensorRT/RIFE, et toute la qualité objective. Tout chiffre encore marqué **[H]** reste une hypothèse.

Unités : pixels, kbit/s, secondes (`std::chrono`), VRAM en Mio (1 Gio = 1 024 Mio).

---

## 1. Vue d'ensemble

```
            CPU                                 GPU (RTX 5070, un seul contexte CUDA primaire)
 ┌──────────────────┐
 │ T0 Démux         │ paquets vidéo   ┌──────────────────────────┐
 │ libavformat +    ├────────────────►│ T1 Décodage              │  NVDEC via hwaccel FFmpeg (AV_PIX_FMT_CUDA)
 │ E/S Win32 (049)  │                 │                          │
 │                  │  file bornée    │  + conversion couleur    │  NV12/P010 → format pivot (noyau CUDA)
 │                  │                 │  + rotation / désentrel. │  flux S_dec
 │                  │                 └────────────┬─────────────┘
 │                  │                              │ VideoFrame (surface du pool source + événement CUDA)
 │                  │                 ┌────────────▼─────────────┐
 │                  │                 │ T2 Traitement            │  [Interpolation → 60 i/s si source < 50 i/s]
 │                  │                 │  chaîne d'IFrameProcessor│  → Super-résolution RTX Video SDK (→ 3840×2160)
 │                  │                 │                          │  → [SDR→HDR10, plus tard] → pivot → NV12/P010
 │                  │                 └────────────┬─────────────┘  flux S_proc
 │                  │                              │ VideoFrame (surface du pool 4K, déjà enregistrée NVENC)
 │                  │                 ┌────────────▼─────────────┐
 │                  │                 │ T3 Encodage NVENC        │  HEVC / AV1, 8 / 10 bits, HDR10 prévu
 │                  │                 │  (API Video Codec SDK)   │  mode asynchrone, événements Win32
 │                  │                 └────────────┬─────────────┘
 │                  │ paquets audio /              │ paquets compressés (mémoire CPU, quelques Mio/s)
 │                  │ sous-titres     ┌────────────▼─────────────┐
 │                  ├────────────────►│ T4 Mux libavformat       │  MP4 (bascule MKV), écrit au chemin exact
 └──────────────────┘  file bornée    │  copie sans réencodage   │  fourni par l'application (ADR-049)
                                      └──────────────────────────┘
 T5 Rapporteur : agrège compteurs et erreurs, appelle l'observateur de l'application (progression 4 fois/s, ADR-051).
```

Seuls transits CPU↔GPU : les **paquets compressés** (entrée et sortie, inévitables et petits) et, en **repli logiciel** uniquement, l'envoi des images décodées par le CPU (via mémoire épinglée). Aucune image ne revient sur le CPU dans le chemin nominal.

---

## 2. Décisions clés (ADR du pipeline)

Numérotation définitive dans la plage **ADR-040 et plus**, réservée au pipeline (`docs/architecture/index.md` en-tête) ; correspondance avec la v0.1 : P01→040, P02→041 … P11→050 ; ADR-051 est nouveau. Licences : à valider par secdevops dans `docs/security/licences.md` (ENH-006).


## Sommaire du pipeline (ménage docs du 2026-10-29)

- ADR-040 à ADR-052 : voir [architecture/index.md](../architecture/index.md)
- § 3 Conception interne : [pipeline-conception.md](pipeline-conception.md)
- § 4-5 Préréglages, budget VRAM et vitesse : [pipeline-vram-vitesse.md](pipeline-vram-vitesse.md)
- § 6 Contrat avec l'application : [pipeline-contrat-api.md](pipeline-contrat-api.md)
- § 7-10 Reprise, extensibilité, risques, à vérifier : [pipeline-reprise-risques.md](pipeline-reprise-risques.md)
