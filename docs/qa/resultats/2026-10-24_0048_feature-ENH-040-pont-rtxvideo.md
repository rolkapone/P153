# Tests GPU locaux - 2026-10-24_0048

- Resultat : **VERT** (code de sortie 0)
- Commit : eda8539 (branche feature-ENH-040-pont-rtxvideo)
- Preset : local-gpu
- GPU : NVIDIA GeForce RTX 5070
- Pilote : 617.42
- CapaciteCalcul : 12.0
- VramTotaleMiB : 12227
- VramUtiliseeDepartMiB : 1707
- NVCC : V13.4.92

## Prerequis

| Element | Statut | Detail |
|---|---|---|
| Carte NVIDIA | OK | NVIDIA GeForce RTX 5070, capacite de calcul 12.0 (>= 12.0 requis), 12227 Mio |
| Pilote | OK | 617.42 (>= 610.00 requis) |
| VRAM au depart | AVERTISSEMENT | 1707 Mio deja utilises (seuil 1536 Mio) |
| CUDA Toolkit | OK | nvcc release 13.4 V13.4.92 (epingle : 13.4) |
| SDK rtx-video-sdk | OK | present |
| SDK video-codec-sdk | OK | present |
| SDK optical-flow-sdk | OK | present |
| SDK tensorrt | OK | present |
| FFmpeg LGPL (shared) | OK | en-tetes presents |
| FFmpeg : licence LGPL (check-ffmpeg-license) | OK | aucune option GPL/nonfree |
| Espace disque | OK | 355,2 Gio libres (>= 10) |

## Tests (CTest, label gpu)

- 1/31 Test  #89: GpuSmoke.CarteBlackwellVisible ...............................................   Passed    0.05 sec
- 2/31 Test  #90: ENH008Nvdec.UnSeulContexteCuda_PartageAvecFfmpeg .............................   Passed    0.14 sec
- 3/31 Test  #91: ENH008Nvdec.ModulesPiloteChargesDepuisSystem32 ...............................   Passed    0.12 sec
- 4/31 Test  #92: ENH008Nvdec.PixelsNvdecIdentiquesAuDecodageLogiciel ..........................   Passed    0.44 sec
- 5/31 Test  #93: ENH008Nvdec.RepliProRes_JournaliseEtEnvoyeSurGpu .............................   Passed    0.35 sec
- 6/31 Test  #94: ENH008Nvdec.Vitesse_Et_Vram_BbbFast ..........................................   Passed    0.75 sec
- 7/31 Test  #95: ENH008Nvdec.DixFichiersALaSuite_SansFuite ....................................   Passed    7.05 sec
- 8/31 Test  #96: ENH008Nvdec.CasLimites_SansPlantage_VramLiberee ..............................   Passed    0.69 sec
- 9/31 Test  #97: ENH008Qa.RepliFfv1_MessageAuJournal ..........................................   Passed    0.55 sec
- 10/31 Test  #98: ENH008Qa.Annulation_DixFois_VramRevenueAuRepos ...............................   Passed    1.42 sec
- 11/31 Test  #99: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_h264 ........   Passed    0.46 sec
- 12/31 Test #100: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_hevc10 ......   Passed    0.60 sec
- 13/31 Test #101: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mkv_av1_10 ......   Passed    0.59 sec
- 14/31 Test #102: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_webm_vp9 ........   Passed    0.52 sec
- 15/31 Test #103: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_ts_mpeg2 ........   Passed    0.34 sec
- 16/31 Test #104: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_avi_h264 ........   Passed    0.48 sec
- 17/31 Test #105: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_h264 ........   Passed    0.44 sec
- 18/31 Test #106: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_prores_hq ...   Passed    1.16 sec
- 19/31 Test #107: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_qa ..........   Passed    0.89 sec
- 20/31 Test #108: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_ffv1 ........   Passed    4.82 sec
- 21/31 Test #109: ENH040.Pont_AucunCudartImporte_ExportsAttendus ...............................   Passed    0.14 sec
- 22/31 Test #110: ENH040.DllFonction_CopieRel_SigneeNvidia .....................................   Passed    0.52 sec
- 23/31 Test #111: ENH040.ModulesEtDllDeFonction_Provenance .....................................   Passed    0.78 sec
- 24/31 Test #112: ENH040.MiseAJourNgx_ProcessusBloque_Trace ....................................   Passed    0.72 sec
- 25/31 Test #113: ENH040.NgxNEcritNiJournalNiMiseAJour .........................................   Passed    0.78 sec
- 26/31 Test #114: ENH040.Vram_EffetQ4_SousLeBudget .............................................   Passed    0.96 sec
- 27/31 Test #115: ENH040.DixCycles_CreationDestruction_SansDerive ..............................   Passed    6.29 sec
- 28/31 Test #116: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits8 .................   Passed    0.76 sec
- 29/31 Test #117: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits10 ................   Passed    0.77 sec
- 30/31 Test #118: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits8 .................................   Passed    6.71 sec
- 31/31 Test #119: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits10 ................................   Passed    6.44 sec

## Mesures

- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)
- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)

