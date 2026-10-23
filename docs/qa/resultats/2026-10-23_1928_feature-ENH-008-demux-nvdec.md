# Tests GPU locaux - 2026-10-23_1928

- Resultat : **VERT** (code de sortie 0)
- Commit : 608c315 (branche feature-ENH-008-demux-nvdec)
- Preset : local-gpu
- GPU : NVIDIA GeForce RTX 5070
- Pilote : 617.42
- CapaciteCalcul : 12.0
- VramTotaleMiB : 12227
- VramUtiliseeDepartMiB : 2311
- NVCC : V13.4.92

## Prerequis

| Element | Statut | Detail |
|---|---|---|
| Carte NVIDIA | OK | NVIDIA GeForce RTX 5070, capacite de calcul 12.0 (>= 12.0 requis), 12227 Mio |
| Pilote | OK | 617.42 (>= 610.00 requis) |
| VRAM au depart | AVERTISSEMENT | 2311 Mio deja utilises (seuil 1536 Mio) |
| CUDA Toolkit | OK | nvcc release 13.4 V13.4.92 (epingle : 13.4) |
| SDK rtx-video-sdk | OK | present |
| SDK video-codec-sdk | OK | present |
| SDK optical-flow-sdk | OK | present |
| SDK tensorrt | OK | present |
| FFmpeg LGPL (shared) | OK | en-tetes presents |
| FFmpeg : licence LGPL (check-ffmpeg-license) | OK | aucune option GPL/nonfree |
| Espace disque | OK | 371 Gio libres (>= 10) |

## Tests (CTest, label gpu)

- 1/20 Test #51: GpuSmoke.CarteBlackwellVisible ...............................................   Passed    0.04 sec
- 2/20 Test #52: ENH008Nvdec.UnSeulContexteCuda_PartageAvecFfmpeg .............................   Passed    0.15 sec
- 3/20 Test #53: ENH008Nvdec.ModulesPiloteChargesDepuisSystem32 ...............................   Passed    0.12 sec
- 4/20 Test #54: ENH008Nvdec.PixelsNvdecIdentiquesAuDecodageLogiciel ..........................   Passed    0.43 sec
- 5/20 Test #55: ENH008Nvdec.RepliProRes_JournaliseEtEnvoyeSurGpu .............................   Passed    0.36 sec
- 6/20 Test #56: ENH008Nvdec.Vitesse_Et_Vram_BbbFast ..........................................   Passed    0.81 sec
- 7/20 Test #57: ENH008Nvdec.DixFichiersALaSuite_SansFuite ....................................   Passed    7.49 sec
- 8/20 Test #58: ENH008Nvdec.CasLimites_SansPlantage_VramLiberee ..............................   Passed    0.70 sec
- 9/20 Test #59: ENH008Qa.RepliFfv1_MessageAuJournal ..........................................   Passed    0.54 sec
- 10/20 Test #60: ENH008Qa.Annulation_DixFois_VramRevenueAuRepos ...............................   Passed    1.44 sec
- 11/20 Test #61: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_h264 ........   Passed    0.46 sec
- 12/20 Test #62: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_hevc10 ......   Passed    0.62 sec
- 13/20 Test #63: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mkv_av1_10 ......   Passed    0.60 sec
- 14/20 Test #64: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_webm_vp9 ........   Passed    0.53 sec
- 15/20 Test #65: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_ts_mpeg2 ........   Passed    0.37 sec
- 16/20 Test #66: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_avi_h264 ........   Passed    0.49 sec
- 17/20 Test #67: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_h264 ........   Passed    0.50 sec
- 18/20 Test #68: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_prores_hq ...   Passed    1.16 sec
- 19/20 Test #69: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_qa ..........   Passed    0.89 sec
- 20/20 Test #70: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_ffv1 ........   Passed    4.94 sec

## Mesures

- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)
- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)

