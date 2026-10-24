# Tests GPU locaux - 2026-10-24_0110

- Resultat : **VERT** (code de sortie 0)
- Commit : 9bdc76f (branche feature-ENH-040-pont-rtxvideo)
- Preset : local-gpu
- GPU : NVIDIA GeForce RTX 5070
- Pilote : 617.42
- CapaciteCalcul : 12.0
- VramTotaleMiB : 12227
- VramUtiliseeDepartMiB : 2386
- NVCC : V13.4.92

## Prerequis

| Element | Statut | Detail |
|---|---|---|
| Carte NVIDIA | OK | NVIDIA GeForce RTX 5070, capacite de calcul 12.0 (>= 12.0 requis), 12227 Mio |
| Pilote | OK | 617.42 (>= 610.00 requis) |
| VRAM au depart | AVERTISSEMENT | 2386 Mio deja utilises (seuil 1536 Mio) |
| CUDA Toolkit | OK | nvcc release 13.4 V13.4.92 (epingle : 13.4) |
| SDK rtx-video-sdk | OK | present |
| SDK video-codec-sdk | OK | present |
| SDK optical-flow-sdk | OK | present |
| SDK tensorrt | OK | present |
| FFmpeg LGPL (shared) | OK | en-tetes presents |
| FFmpeg : licence LGPL (check-ffmpeg-license) | OK | aucune option GPL/nonfree |
| Espace disque | OK | 354,6 Gio libres (>= 10) |

## Tests (CTest, label gpu)

- 1/32 Test  #90: GpuSmoke.CarteBlackwellVisible ...............................................   Passed    0.04 sec
- 2/32 Test  #91: ENH008Nvdec.UnSeulContexteCuda_PartageAvecFfmpeg .............................   Passed    0.11 sec
- 3/32 Test  #92: ENH008Nvdec.ModulesPiloteChargesDepuisSystem32 ...............................   Passed    0.11 sec
- 4/32 Test  #93: ENH008Nvdec.PixelsNvdecIdentiquesAuDecodageLogiciel ..........................   Passed    0.40 sec
- 5/32 Test  #94: ENH008Nvdec.RepliProRes_JournaliseEtEnvoyeSurGpu .............................   Passed    0.32 sec
- 6/32 Test  #95: ENH008Nvdec.Vitesse_Et_Vram_BbbFast ..........................................   Passed    0.76 sec
- 7/32 Test  #96: ENH008Nvdec.DixFichiersALaSuite_SansFuite ....................................   Passed    6.71 sec
- 8/32 Test  #97: ENH008Nvdec.CasLimites_SansPlantage_VramLiberee ..............................   Passed    0.68 sec
- 9/32 Test  #98: ENH008Qa.RepliFfv1_MessageAuJournal ..........................................   Passed    0.52 sec
- 10/32 Test  #99: ENH008Qa.Annulation_DixFois_VramRevenueAuRepos ...............................   Passed    1.37 sec
- 11/32 Test #100: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_h264 ........   Passed    0.43 sec
- 12/32 Test #101: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_hevc10 ......   Passed    0.56 sec
- 13/32 Test #102: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mkv_av1_10 ......   Passed    0.54 sec
- 14/32 Test #103: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_webm_vp9 ........   Passed    0.51 sec
- 15/32 Test #104: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_ts_mpeg2 ........   Passed    0.33 sec
- 16/32 Test #105: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_avi_h264 ........   Passed    0.45 sec
- 17/32 Test #106: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_h264 ........   Passed    0.43 sec
- 18/32 Test #107: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_prores_hq ...   Passed    1.14 sec
- 19/32 Test #108: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_qa ..........   Passed    0.86 sec
- 20/32 Test #109: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_ffv1 ........   Passed    4.79 sec
- 21/32 Test #110: ENH040.Pont_AucunCudartImporte_ExportsAttendus ...............................   Passed    0.11 sec
- 22/32 Test #111: ENH040.DllFonction_CopieRel_SigneeNvidia .....................................   Passed    0.49 sec
- 23/32 Test #112: ENH040.ModulesEtDllDeFonction_Provenance .....................................   Passed    0.71 sec
- 24/32 Test #113: ENH040.MiseAJourNgx_ProcessusBloque_Trace ....................................   Passed    0.67 sec
- 25/32 Test #114: ENH040.InitNgxEchouee_PontOublieLeRappel_PuisCreationReussie .................   Passed    0.92 sec
- 26/32 Test #115: ENH040.NgxNEcritNiJournalNiMiseAJour .........................................   Passed    0.74 sec
- 27/32 Test #116: ENH040.Vram_EffetQ4_SousLeBudget .............................................   Passed    0.95 sec
- 28/32 Test #117: ENH040.DixCycles_CreationDestruction_SansDerive ..............................   Passed    6.17 sec
- 29/32 Test #118: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits8 .................   Passed    0.71 sec
- 30/32 Test #119: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits10 ................   Passed    0.73 sec
- 31/32 Test #120: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits8 .................................   Passed    6.58 sec
- 32/32 Test #121: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits10 ................................   Passed    6.28 sec

## Mesures

- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)
- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)

