# Tests GPU locaux - 2026-10-23_1538

- Resultat : **VERT** (code de sortie 0)
- Commit : a9291e1 (branche feature-ENH-008-demux-nvdec)
- Preset : local-gpu
- GPU : NVIDIA GeForce RTX 5070
- Pilote : 617.42
- CapaciteCalcul : 12.0
- VramTotaleMiB : 12227
- VramUtiliseeDepartMiB : 1554
- NVCC : V13.4.92

## Prerequis

| Element | Statut | Detail |
|---|---|---|
| Carte NVIDIA | OK | NVIDIA GeForce RTX 5070, capacite de calcul 12.0 (>= 12.0 requis), 12227 Mio |
| Pilote | OK | 617.42 (>= 610.00 requis) |
| VRAM au depart | AVERTISSEMENT | 1554 Mio deja utilises (seuil 1536 Mio) |
| CUDA Toolkit | OK | nvcc release 13.4 V13.4.92 (epingle : 13.4) |
| SDK rtx-video-sdk | OK | present |
| SDK video-codec-sdk | OK | present |
| SDK optical-flow-sdk | OK | present |
| SDK tensorrt | OK | present |
| FFmpeg LGPL (shared) | OK | en-tetes presents |
| FFmpeg : licence LGPL (check-ffmpeg-license) | OK | aucune option GPL/nonfree |
| Espace disque | OK | 371,1 Gio libres (>= 10) |

## Tests (CTest, label gpu)

- 1/18 Test #51: GpuSmoke.CarteBlackwellVisible ...............................................   Passed    0.12 sec
- 2/18 Test #52: ENH008Nvdec.UnSeulContexteCuda_PartageAvecFfmpeg .............................   Passed    0.17 sec
- 3/18 Test #53: ENH008Nvdec.ModulesPiloteChargesDepuisSystem32 ...............................   Passed    0.13 sec
- 4/18 Test #54: ENH008Nvdec.PixelsNvdecIdentiquesAuDecodageLogiciel ..........................   Passed    0.44 sec
- 5/18 Test #55: ENH008Nvdec.RepliProRes_JournaliseEtEnvoyeSurGpu .............................   Passed    0.36 sec
- 6/18 Test #56: ENH008Nvdec.Vitesse_Et_Vram_BbbFast ..........................................   Passed    0.78 sec
- 7/18 Test #57: ENH008Nvdec.DixFichiersALaSuite_SansFuite ....................................   Passed    6.86 sec
- 8/18 Test #58: ENH008Nvdec.CasLimites_SansPlantage_VramLiberee ..............................   Passed    0.68 sec
- 9/18 Test #59: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_h264 ........   Passed    0.47 sec
- 10/18 Test #60: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_hevc10 ......   Passed    0.65 sec
- 11/18 Test #61: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mkv_av1_10 ......   Passed    0.57 sec
- 12/18 Test #62: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_webm_vp9 ........   Passed    0.53 sec
- 13/18 Test #63: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_ts_mpeg2 ........   Passed    0.37 sec
- 14/18 Test #64: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_avi_h264 ........   Passed    0.52 sec
- 15/18 Test #65: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_h264 ........   Passed    0.45 sec
- 16/18 Test #66: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_prores_hq ...   Passed    1.15 sec
- 17/18 Test #67: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_qa ..........   Passed    0.90 sec
- 18/18 Test #68: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_ffv1 ........   Passed    4.82 sec

## Mesures

- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)
- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)

