# Tests GPU locaux - 2026-10-24_1919

- Resultat : **VERT** (code de sortie 0)
- Commit : 373ced9 (branche feature-ENH-047-enhance-bench)
- Preset : local-gpu
- GPU : NVIDIA GeForce RTX 5070
- Pilote : 617.42
- CapaciteCalcul : 12.0
- VramTotaleMiB : 12227
- VramUtiliseeDepartMiB : 2084
- NVCC : V13.4.92

## Prerequis

| Element | Statut | Detail |
|---|---|---|
| Carte NVIDIA | OK | NVIDIA GeForce RTX 5070, capacite de calcul 12.0 (>= 12.0 requis), 12227 Mio |
| Pilote | OK | 617.42 (>= 610.00 requis) |
| VRAM au depart | AVERTISSEMENT | 2084 Mio deja utilises (seuil 1536 Mio) |
| CUDA Toolkit | OK | nvcc release 13.4 V13.4.92 (epingle : 13.4) |
| SDK rtx-video-sdk | OK | present |
| SDK video-codec-sdk | OK | present |
| SDK optical-flow-sdk | OK | present |
| SDK tensorrt | OK | present |
| FFmpeg LGPL (shared) | OK | en-tetes presents |
| FFmpeg : licence LGPL (check-ffmpeg-license) | OK | aucune option GPL/nonfree |
| Espace disque | OK | 312 Gio libres (>= 10) |

## Tests (CTest, label gpu)

- 1/41 Test #211: GpuSmoke.CarteBlackwellVisible ...............................................   Passed    0.05 sec
- 2/41 Test #212: ENH008Nvdec.UnSeulContexteCuda_PartageAvecFfmpeg .............................   Passed    0.13 sec
- 3/41 Test #213: ENH008Nvdec.ModulesPiloteChargesDepuisSystem32 ...............................   Passed    0.11 sec
- 4/41 Test #214: ENH008Nvdec.PixelsNvdecIdentiquesAuDecodageLogiciel ..........................   Passed    0.42 sec
- 5/41 Test #215: ENH008Nvdec.RepliProRes_JournaliseEtEnvoyeSurGpu .............................   Passed    0.34 sec
- 6/41 Test #216: ENH008Nvdec.Vitesse_Et_Vram_BbbFast ..........................................   Passed    0.77 sec
- 7/41 Test #217: ENH008Nvdec.DixFichiersALaSuite_SansFuite ....................................   Passed    6.84 sec
- 8/41 Test #218: ENH008Nvdec.CasLimites_SansPlantage_VramLiberee ..............................   Passed    0.68 sec
- 9/41 Test #219: ENH008Qa.RepliFfv1_MessageAuJournal ..........................................   Passed    0.54 sec
- 10/41 Test #220: ENH008Qa.Annulation_DixFois_VramRevenueAuRepos ...............................   Passed    1.41 sec
- 11/41 Test #221: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_h264 ........   Passed    0.44 sec
- 12/41 Test #222: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mp4_hevc10 ......   Passed    0.57 sec
- 13/41 Test #223: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mkv_av1_10 ......   Passed    0.55 sec
- 14/41 Test #224: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_webm_vp9 ........   Passed    0.50 sec
- 15/41 Test #225: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_ts_mpeg2 ........   Passed    0.33 sec
- 16/41 Test #226: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_avi_h264 ........   Passed    0.47 sec
- 17/41 Test #227: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_h264 ........   Passed    0.44 sec
- 18/41 Test #228: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/core_mov_prores_hq ...   Passed    1.14 sec
- 19/41 Test #229: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_qa ..........   Passed    0.86 sec
- 20/41 Test #230: Noyau/ENH008Matrix.ImagesEnMemoireGpu_NombreEgalFfprobe/BBB_FAST_ffv1 ........   Passed    5.06 sec
- 21/41 Test #231: ENH040.Pont_AucunCudartImporte_ExportsAttendus ...............................   Passed    0.12 sec
- 22/41 Test #232: ENH040.DllFonction_CopieRel_SigneeNvidia .....................................   Passed    0.55 sec
- 23/41 Test #233: ENH040.ModulesEtDllDeFonction_Provenance .....................................   Passed    0.68 sec
- 24/41 Test #234: ENH040.MiseAJourNgx_ProcessusBloque_Trace ....................................   Passed    0.68 sec
- 25/41 Test #235: ENH040.InitNgxEchouee_PontOublieLeRappel_PuisCreationReussie .................   Passed    0.89 sec
- 26/41 Test #236: ENH040.NgxNEcritNiJournalNiMiseAJour .........................................   Passed    0.70 sec
- 27/41 Test #237: ENH040.Vram_EffetQ4_SousLeBudget .............................................   Passed    0.88 sec
- 28/41 Test #238: ENH040.DixCycles_CreationDestruction_SansDerive ..............................   Passed    7.10 sec
- 29/41 Test #239: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits8 .................   Passed    0.76 sec
- 30/41 Test #240: Formats/ENH040Bits.Synthetique720pVers4K_Q4DiffereDeQ0/Bits10 ................   Passed    0.77 sec
- 31/41 Test #241: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits8 .................................   Passed    6.14 sec
- 32/41 Test #242: Formats/ENH040Bits.Debit_Q4_720pVers4K/Bits10 ................................   Passed    5.80 sec
- 33/41 Test #243: ENH047Bench.Decode_BbbFast_JsonCoherent_ImagesEgalFfprobe ....................   Passed    0.94 sec
- 34/41 Test #244: ENH047Bench.Decode_Repeat10_SansDerive .......................................   Passed    1.50 sec
- 35/41 Test #245: ENH047Bench.Decode_RepliLogiciel_Ffv1 ........................................   Passed    0.73 sec
- 36/41 Test #246: ENH047Bench.CodesDeSortie_EntreesIllisibles ..................................   Passed    0.35 sec
- 37/41 Test #247: ENH047Bench.CodesDeSortie_DecodagePartiel_Code2_SaufAllowCorrupt .............   Passed    0.93 sec
- 38/41 Test #248: ENH047Bench.Decode_VideoCorrompueNonVide_Code2_JsonAvecErreur ................   Passed    0.18 sec
- 39/41 Test #249: ENH047Bench.SortieDesignantLEntree_Code1_EntreeIntacte_MemeAvecForce .........   Passed    0.07 sec
- 40/41 Test #250: ENH047Bench.VsrImage_Sortie4K_Q4DiffereDeQ0_VramSousLeSeuil ..................   Passed    2.20 sec
- 41/41 Test #251: ENH047Bench.VsrImage_BmpMalforme_Code2_SansSortie ............................   Passed    0.14 sec

## Mesures

- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)
- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)

