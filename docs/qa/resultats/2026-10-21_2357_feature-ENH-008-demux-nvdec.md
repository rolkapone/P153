# Tests GPU locaux - 2026-10-21_2357

- Resultat : **VERT** (code de sortie 0)
- Commit : 592f46e (branche feature-ENH-008-demux-nvdec)
- Preset : local-gpu
- GPU : NVIDIA GeForce RTX 5070
- Pilote : 617.42
- CapaciteCalcul : 12.0
- VramTotaleMiB : 12227
- VramUtiliseeDepartMiB : 2284
- NVCC : V13.4.92

## Prerequis

| Element | Statut | Detail |
|---|---|---|
| Carte NVIDIA | OK | NVIDIA GeForce RTX 5070, capacite de calcul 12.0 (>= 12.0 requis), 12227 Mio |
| Pilote | OK | 617.42 (>= 610.00 requis) |
| VRAM au depart | AVERTISSEMENT | 2284 Mio deja utilises (seuil 1536 Mio) |
| CUDA Toolkit | OK | nvcc release 13.4 V13.4.92 (epingle : 13.4) |
| SDK rtx-video-sdk | OK | present |
| SDK video-codec-sdk | OK | present |
| SDK optical-flow-sdk | OK | present |
| SDK tensorrt | OK | present |
| FFmpeg LGPL (shared) | OK | en-tetes presents |
| FFmpeg : licence LGPL (check-ffmpeg-license) | OK | aucune option GPL/nonfree |
| Espace disque | OK | 375,2 Gio libres (>= 10) |

## Tests (CTest, label gpu)

- 1/18 Test #47: GpuSmoke.CarteBlackwellVisible ...............................................   Passed    0.04 sec
- 2/18 Test #48: ENH008Nvdec.UnSeulContexteCuda_PartageAvecFfmpeg .............................   Passed    0.12 sec
- 3/18 Test #49: ENH008Nvdec.ModulesPiloteChargesDepuisSystem32 ...............................   Passed    0.10 sec
- 4/18 Test #50: ENH008Nvdec.PixelsNvdecIdentiquesAuDecodageLogiciel ..........................   Passed    0.39 sec
- 5/18 Test #51: ENH008Nvdec.RepliProRes_JournaliseEtEnvoyeSurGpu .............................   Passed    0.30 sec
- 6/18 Test #52: ENH008Nvdec.Vitesse_Et_Vram_BbbFast ..........................................   Passed    0.76 sec
- 7/18 Test #53: ENH008Nvdec.DixFichiersALaSuite_SansFuite ....................................   Passed    6.92 sec
- 8/18 Test #54: ENH008Nvdec.CasLimites_SansPlantage_VramLiberee ..............................   Passed    0.66 sec
- 9/18 Test #55: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_h264 ........   Passed    0.43 sec
- 10/18 Test #56: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_hevc10 ......   Passed    0.54 sec
- 11/18 Test #57: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mkv_av1_10 ......   Passed    0.51 sec
- 12/18 Test #58: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_webm_vp9 ........   Passed    0.48 sec
- 13/18 Test #59: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_ts_mpeg2 ........   Passed    0.31 sec
- 14/18 Test #60: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_avi_h264 ........   Passed    0.45 sec
- 15/18 Test #61: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_h264 ........   Passed    0.41 sec
- 16/18 Test #62: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_prores_hq ...   Passed    1.12 sec
- 17/18 Test #63: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_qa ..........   Passed    0.84 sec
- 18/18 Test #64: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_ffv1 ........   Passed    4.73 sec

## Mesures

- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)
- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)

