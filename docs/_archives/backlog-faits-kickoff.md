> Extrait de l'ancien `docs/backlog.md` (ménage docs du 2026-10-29), texte inchangé. Index : [backlog](../backlog.md).

## Faits techniques établis par le kickoff (reflétés dans les stories)

| Sujet | Fait | Où dans le backlog |
|---|---|---|
| Chaîne mesurée (RIFE mixte FP16/FP32 720p -> VSR -> NVENC, 4K60) | Équilibré **103-112 i/s** (HEVC 108 / AV1 111-112), Qualité max **36,5 i/s (HEVC) / 51,5 (AV1)**, Rapide **219 i/s** ; VRAM pic **1,5-2,1 Gio** ; **NVENC est le goulot** (un seul moteur) ; 0 Mio de dérive sur 10 jobs | ENH-009, 010, 011, 012, 014 |
| Interpolation | **RIFE 4.25 > FRUC** (+2 à +15 VMAF ; FRUC abandonne dès ±2 % de grain et fait pire que la duplication sur le texte) ; **FRUC = repli local non livrable** (SEC-04) ; RIFE 4K après SR écarté (32 i/s, 3,9 Gio) | ENH-012, 043, 044, 045 |
| RTX Video SDK 1.1.0 | **Entrée RGB seulement** (conversions YUV <-> RGB à notre charge), 10 bits OK, 5 niveaux de qualité (q4 Ultra : 322 i/s, 188 Mio) ; **DLL pont obligatoire** (B4 : lib NGX compilée CUDA 11.8, plante avec le runtime 13) ; **NGX Update Module** à neutraliser et à tester (SEC-01) | ENH-040, 041, 042 |
| TensorRT 11.4 | Plus de `--fp16` : conversion ONNX **mixte** (convolutions FP16, grille et `GridSample` FP32 ; FP16 complet = -4 à -8 VMAF) ; moteur ~85 s au premier traitement (8,5 s avec cache de timing), chargement 0,3-0,6 s | ENH-043, 044 |
| Coupures de plan | Les images générées à cheval sur une coupure sont ratées (VMAF min ~0-5) | ENH-046 |
| Dépendances | Qt 6.11.2 sans Network (overlay ports vcpkg) ; FFmpeg BtbN LGPL = **développement seulement** ; **SEC-06 : FFmpeg vcpkg minimal sans réseau obligatoire avant ENH-030** (exceptions dev E3/E3c/E3d/E3e échues le 2027-01-21) | ENH-048, 028, 030 |
| QA | `enhance-bench.exe` demandé (protocole §4.3) | ENH-047 |
| Machine | ~2 à 3,8 Gio de VRAM déjà occupés au repos par le bureau (Docker, iCUE, navigateur) : mesures en différence ; fermer les applications graphiques avant une mesure officielle | ENH-014, QA |
