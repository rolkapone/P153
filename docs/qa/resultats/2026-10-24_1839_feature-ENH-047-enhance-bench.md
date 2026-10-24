# Tests GPU locaux - 2026-10-24_1839

- Resultat : **ROUGE** (code de sortie 1)
- Commit : bc5bebb (branche feature-ENH-047-enhance-bench)
- Preset : local-gpu
- GPU : NVIDIA GeForce RTX 5070
- Pilote : 617.42
- CapaciteCalcul : 12.0
- VramTotaleMiB : 12227
- VramUtiliseeDepartMiB : 1855
- NVCC : V13.4.92

## Prerequis

| Element | Statut | Detail |
|---|---|---|
| Carte NVIDIA | OK | NVIDIA GeForce RTX 5070, capacite de calcul 12.0 (>= 12.0 requis), 12227 Mio |
| Pilote | OK | 617.42 (>= 610.00 requis) |
| VRAM au depart | AVERTISSEMENT | 1855 Mio deja utilises (seuil 1536 Mio) |
| CUDA Toolkit | OK | nvcc release 13.4 V13.4.92 (epingle : 13.4) |
| SDK rtx-video-sdk | OK | present |
| SDK video-codec-sdk | OK | present |
| SDK optical-flow-sdk | OK | present |
| SDK tensorrt | OK | present |
| FFmpeg LGPL (shared) | OK | en-tetes presents |
| FFmpeg : licence LGPL (check-ffmpeg-license) | OK | aucune option GPL/nonfree |
| Espace disque | OK | 312 Gio libres (>= 10) |

## Tests (CTest, label gpu)

- 1/38 Test #204: GpuSmoke.CarteBlackwellVisible ...............................................   Passed    0.07 sec
- 2/38 Test #205: ENH008Nvdec.UnSeulContexteCuda_PartageAvecFfmpeg .............................   Passed    0.23 sec
- 3/38 Test #206: ENH008Nvdec.ModulesPiloteChargesDepuisSystem32 ...............................   Passed    0.13 sec
- 4/38 Test #207: ENH008Nvdec.PixelsNvdecIdentiquesAuDecodageLogiciel ..........................   Passed    0.47 sec
- 5/38 Test #208: ENH008Nvdec.RepliProRes_JournaliseEtEnvoyeSurGpu .............................   Passed    0.34 sec
- 6/38 Test #209: ENH008Nvdec.Vitesse_Et_Vram_BbbFast ..........................................   Passed    0.77 sec
- 7/38 Test #210: ENH008Nvdec.DixFichiersALaSuite_SansFuite ....................................   Passed    6.74 sec
- 8/38 Test #211: ENH008Nvdec.CasLimites_SansPlantage_VramLiberee ..............................   Passed    0.69 sec
- 9/38 Test #212: ENH008Qa.RepliFfv1_MessageAuJournal ..........................................   Passed    0.53 sec
- 10/38 Test #213: ENH008Qa.Annulation_DixFois_VramRevenueAuRepos ...............................   Passed    1.39 sec
- 11/38 Test #214: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_h264 ........   Passed    0.45 sec
- 12/38 Test #215: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_hevc10 ......   Passed    0.59 sec
- 13/38 Test #216: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mkv_av1_10 ......   Passed    0.57 sec
- 14/38 Test #217: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_webm_vp9 ........   Passed    0.52 sec
- 15/38 Test #218: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_ts_mpeg2 ........   Passed    0.34 sec
- 16/38 Test #219: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_avi_h264 ........   Passed    0.48 sec
- 17/38 Test #220: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_h264 ........   Passed    0.44 sec
- 18/38 Test #221: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_prores_hq ...   Passed    1.16 sec
- 19/38 Test #222: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_qa ..........   Passed    0.90 sec
- 20/38 Test #223: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_ffv1 ........   Passed    4.83 sec
- 21/38 Test #224: ENH040.Pont_AucunCudartImporte_ExportsAttendus ...............................   Passed    0.13 sec
- 22/38 Test #225: ENH040.DllFonction_CopieRel_SigneeNvidia .....................................   Passed    0.54 sec
- 23/38 Test #226: ENH040.ModulesEtDllDeFonction_Provenance .....................................   Passed    0.75 sec
- 24/38 Test #227: ENH040.MiseAJourNgx_ProcessusBloque_Trace ....................................   Passed    0.66 sec
- 25/38 Test #228: ENH040.InitNgxEchouee_PontOublieLeRappel_PuisCreationReussie .................   Passed    0.93 sec
- 26/38 Test #229: ENH040.NgxNEcritNiJournalNiMiseAJour .........................................   Passed    0.74 sec
- 27/38 Test #230: ENH040.Vram_EffetQ4_SousLeBudget .............................................   Passed    0.98 sec
- 28/38 Test #231: ENH040.DixCycles_CreationDestruction_SansDerive ..............................   Passed    6.25 sec
- 29/38 Test #232: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits8 .................   Passed    0.77 sec
- 30/38 Test #233: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits10 ................   Passed    0.76 sec
- 31/38 Test #234: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits8 .................................   Passed    6.85 sec
- 32/38 Test #235: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits10 ................................***Failed    6.72 sec
- 33/38 Test #236: ENH047Bench.Decode_BbbFast_JsonCoherent_ImagesEgalFfprobe ....................   Passed    0.97 sec
- 34/38 Test #237: ENH047Bench.Decode_Repeat10_SansDerive .......................................   Passed    1.52 sec
- 35/38 Test #238: ENH047Bench.Decode_RepliLogiciel_Ffv1 ........................................   Passed    0.74 sec
- 36/38 Test #239: ENH047Bench.CodesDeSortie_EntreesIllisibles ..................................   Passed    0.36 sec
- 37/38 Test #240: ENH047Bench.VsrImage_Sortie4K_Q4DiffereDeQ0_VramSousLeSeuil ..................   Passed    2.32 sec
- 38/38 Test #241: ENH047Bench.VsrImage_BmpMalforme_Code2_SansSortie ............................   Passed    0.15 sec

## Mesures

- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)
- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)

