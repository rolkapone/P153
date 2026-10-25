# Tests GPU locaux - 2026-10-24_1854

- Resultat : **VERT** (code de sortie 0)
- Commit : 3fa0fbd (branche feature-ENH-047-enhance-bench)
- Preset : local-gpu
- GPU : NVIDIA GeForce RTX 5070
- Pilote : 617.42
- CapaciteCalcul : 12.0
- VramTotaleMiB : 12227
- VramUtiliseeDepartMiB : 1669
- NVCC : V13.4.92

## Prerequis

| Element | Statut | Detail |
|---|---|---|
| Carte NVIDIA | OK | NVIDIA GeForce RTX 5070, capacite de calcul 12.0 (>= 12.0 requis), 12227 Mio |
| Pilote | OK | 617.42 (>= 610.00 requis) |
| VRAM au depart | AVERTISSEMENT | 1669 Mio deja utilises (seuil 1536 Mio) |
| CUDA Toolkit | OK | nvcc release 13.4 V13.4.92 (epingle : 13.4) |
| SDK rtx-video-sdk | OK | present |
| SDK video-codec-sdk | OK | present |
| SDK optical-flow-sdk | OK | present |
| SDK tensorrt | OK | present |
| FFmpeg LGPL (shared) | OK | en-tetes presents |
| FFmpeg : licence LGPL (check-ffmpeg-license) | OK | aucune option GPL/nonfree |
| Espace disque | OK | 312 Gio libres (>= 10) |

## Tests (CTest, label gpu)

- 1/40 Test #210: GpuSmoke.CarteBlackwellVisible ...............................................   Passed    0.04 sec
- 2/40 Test #211: ENH008Nvdec.UnSeulContexteCuda_PartageAvecFfmpeg .............................   Passed    0.13 sec
- 3/40 Test #212: ENH008Nvdec.ModulesPiloteChargesDepuisSystem32 ...............................   Passed    0.10 sec
- 4/40 Test #213: ENH008Nvdec.PixelsNvdecIdentiquesAuDecodageLogiciel ..........................   Passed    0.38 sec
- 5/40 Test #214: ENH008Nvdec.RepliProRes_JournaliseEtEnvoyeSurGpu .............................   Passed    0.31 sec
- 6/40 Test #215: ENH008Nvdec.Vitesse_Et_Vram_BbbFast ..........................................   Passed    0.74 sec
- 7/40 Test #216: ENH008Nvdec.DixFichiersALaSuite_SansFuite ....................................   Passed    6.71 sec
- 8/40 Test #217: ENH008Nvdec.CasLimites_SansPlantage_VramLiberee ..............................   Passed    0.66 sec
- 9/40 Test #218: ENH008Qa.RepliFfv1_MessageAuJournal ..........................................   Passed    0.53 sec
- 10/40 Test #219: ENH008Qa.Annulation_DixFois_VramRevenueAuRepos ...............................   Passed    1.37 sec
- 11/40 Test #220: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_h264 ........   Passed    0.43 sec
- 12/40 Test #221: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_hevc10 ......   Passed    0.57 sec
- 13/40 Test #222: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mkv_av1_10 ......   Passed    0.56 sec
- 14/40 Test #223: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_webm_vp9 ........   Passed    0.53 sec
- 15/40 Test #224: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_ts_mpeg2 ........   Passed    0.33 sec
- 16/40 Test #225: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_avi_h264 ........   Passed    0.46 sec
- 17/40 Test #226: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_h264 ........   Passed    0.41 sec
- 18/40 Test #227: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_prores_hq ...   Passed    1.12 sec
- 19/40 Test #228: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_qa ..........   Passed    0.85 sec
- 20/40 Test #229: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_ffv1 ........   Passed    4.80 sec
- 21/40 Test #230: ENH040.Pont_AucunCudartImporte_ExportsAttendus ...............................   Passed    0.11 sec
- 22/40 Test #231: ENH040.DllFonction_CopieRel_SigneeNvidia .....................................   Passed    0.55 sec
- 23/40 Test #232: ENH040.ModulesEtDllDeFonction_Provenance .....................................   Passed    0.66 sec
- 24/40 Test #233: ENH040.MiseAJourNgx_ProcessusBloque_Trace ....................................   Passed    0.67 sec
- 25/40 Test #234: ENH040.InitNgxEchouee_PontOublieLeRappel_PuisCreationReussie .................   Passed    0.88 sec
- 26/40 Test #235: ENH040.NgxNEcritNiJournalNiMiseAJour .........................................   Passed    0.70 sec
- 27/40 Test #236: ENH040.Vram_EffetQ4_SousLeBudget .............................................   Passed    0.92 sec
- 28/40 Test #237: ENH040.DixCycles_CreationDestruction_SansDerive ..............................   Passed    5.98 sec
- 29/40 Test #238: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits8 .................   Passed    0.73 sec
- 30/40 Test #239: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits10 ................   Passed    0.74 sec
- 31/40 Test #240: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits8 .................................   Passed    6.39 sec
- 32/40 Test #241: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits10 ................................   Passed    6.17 sec
- 33/40 Test #242: ENH047Bench.Decode_BbbFast_JsonCoherent_ImagesEgalFfprobe ....................   Passed    0.92 sec
- 34/40 Test #243: ENH047Bench.Decode_Repeat10_SansDerive .......................................   Passed    1.50 sec
- 35/40 Test #244: ENH047Bench.Decode_RepliLogiciel_Ffv1 ........................................   Passed    0.73 sec
- 36/40 Test #245: ENH047Bench.CodesDeSortie_EntreesIllisibles ..................................   Passed    0.34 sec
- 37/40 Test #246: ENH047Bench.Decode_VideoCorrompueNonVide_Code2_JsonAvecErreur ................   Passed    0.16 sec
- 38/40 Test #247: ENH047Bench.SortieDesignantLEntree_Code1_EntreeIntacte_MemeAvecForce .........   Passed    0.06 sec
- 39/40 Test #248: ENH047Bench.VsrImage_Sortie4K_Q4DiffereDeQ0_VramSousLeSeuil ..................   Passed    2.24 sec
- 40/40 Test #249: ENH047Bench.VsrImage_BmpMalforme_Code2_SansSortie ............................   Passed    0.15 sec

## Mesures

- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)
- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)

