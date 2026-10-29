> Extrait de l'ancien `docs/gpu/architecture-pipeline.md` (ménage docs du 2026-10-29), texte inchangé. Sommaire : [architecture-pipeline.md](architecture-pipeline.md).

## 3. Conception interne

### 3.1 Modules et cibles CMake

```
src/common/include/enh/expected.hpp   alias enh::Expected (ADR-003), cible enh_common partagée avec l'application
src/pipeline/include/enh/pipeline/  en-têtes publics (cible enh_pipeline_api) : seul point d'entrée du developer (§6)
src/pipeline/plan/    plan() pur + ContainerPolicy + tables d'estimation (cible enh_pipeline_plan, CI)
src/pipeline/fake/    FakePipeline, FakeMediaProbe (cible enh_pipeline_fake, CI, utilisée par les tests de l'app)
src/pipeline/core/    C++20 pur, compilé et testé en CI : Pipeline, BoundedQueue, PauseGate,
                      CadencePlanner, ColorResolver, VramBudget, Reporter (cadence 250 ms)
src/pipeline/ffmpeg/  RAII FFmpeg + Win32File/AVIO maison (ADR-049) + FfmpegDemuxer, FfmpegDecoder
                      (hwaccel + logiciel), FfmpegMuxer, FfmpegMediaProbe (probe + vignette)
                      (compilé en CI : démux, mux, sondage et décodage logiciel testables sans GPU)
src/pipeline/cuda/    RAII CUDA, pools de surfaces, noyaux de couleur (.cu, sm_120 + PTX compute_120)
src/pipeline/nvenc/   NvencEncoder
src/pipeline/rtxvideo/RtxVideoUpscaler (+ RtxVideoHdrConverter plus tard)
src/pipeline/interp/  RifeTrtInterpolator, FrucInterpolator
src/pipeline/gpu_factory/ NvidiaStageFactory (assemble les implémentations réelles)
tests/unit/pipeline/  fakes + tests CI ;  tests/gpu/pipeline/  tests et bancs sur la RTX
```
Option CMake `ENHANCE_WITH_GPU` (OFF en CI) : sans elle, les dossiers `cuda/ nvenc/ rtxvideo/ interp/` ne sont pas compilés et `createNvidiaPipeline()` renvoie `ComponentMissing{NvidiaGpu}`. `enh_pipeline_api`, `enh_pipeline_plan`, `enh_pipeline_fake` et `ffmpeg/` ne dépendent ni de CUDA ni des SDK NVIDIA.

### 3.2 Interfaces internes (testables sans GPU)

```cpp
namespace enh::pipeline {

// Image en transit : déplaçable, non copiable. La surface retourne au pool à la destruction (RAII).
struct VideoFrame {
  FrameDesc desc;                 // largeur/hauteur (px), PixelFormat, ColorInfo
  std::int64_t pts;               // dans time_base
  Rational time_base;
  std::int64_t index;             // numéro d'image (source ou sortie)
  FrameFlags flags;               // SceneCut, Generated, Duplicated
  PooledSurface surface;          // mémoire GPU (réel) ou CPU (fakes)
  GpuFence ready;                 // événement CUDA : données valides après ; vide pour les fakes
};

class IFrameSink {               // l'étage suivant (une file bornée en pratique)
 public:
  virtual ~IFrameSink() = default;
  virtual Result<void> push(VideoFrame&& f, std::stop_token st) = 0;  // bloque si plein (back-pressure)
};

class IDemuxer {
 public:
  virtual ~IDemuxer() = default;
  virtual const MediaInfo& info() const = 0;
  virtual Result<std::optional<Packet>> read() = 0;                    // nullopt = fin
  virtual Result<void> seek(std::chrono::duration<double> t) = 0;     // aperçu, reprise
};

class IDecoder {
 public:
  virtual ~IDecoder() = default;
  virtual Result<void> decode(const Packet* pkt, IFrameSink& out, std::stop_token st) = 0; // nullptr = vidange
  virtual DecoderPath path() const = 0;                                // Nvdec | Software (journal)
};

// N images en entrée → M en sortie (interpolation : M ≥ N ; SR, couleur, HDR : M = N).
class IFrameProcessor {
 public:
  virtual ~IFrameProcessor() = default;
  virtual Result<FrameDesc> configure(const FrameDesc& in, const ProcessorSettings& s) = 0;
  virtual Result<void> process(VideoFrame&& in, IFrameSink& out, std::stop_token st) = 0;
  virtual Result<void> flush(IFrameSink& out, std::stop_token st) = 0;
  virtual VramEstimate vramEstimate(const FrameDesc& in) const = 0;   // contrôle avant démarrage
};
class IInterpolator : public IFrameProcessor {};   // marqueurs pour la fabrique et les tests
class IUpscaler     : public IFrameProcessor {};
class IHdrConverter : public IFrameProcessor {};   // plus tard (ENH-032)

class IEncoder {
 public:
  virtual ~IEncoder() = default;
  virtual Result<StreamParameters> configure(const FrameDesc& in, const EncodeSettings& s) = 0; // extradata pour le mux
  virtual Result<void> encode(VideoFrame&& f, IPacketSink& out, std::stop_token st) = 0;
  virtual Result<void> flush(IPacketSink& out) = 0;
};

class IMuxer {
 public:
  virtual ~IMuxer() = default;
  virtual Result<void> open(const std::filesystem::path& output_path, Container c,   // CREATE_NEW (ADR-049)
                            const StreamParameters& video, std::span<const CopiedStream> copied) = 0;
  virtual Result<void> write(Packet&& p) = 0;     // entrelacement par dts (av_interleaved_write_frame)
  virtual Result<void> finalize() = 0;            // fin du conteneur + FlushFileBuffers + fermeture (pas de renommage)
  virtual void abort() noexcept = 0;              // ferme le fichier ; ne supprime rien (l'application s'en charge)
};

class IStageFactory {             // injectée dans le moteur : réelle (NVIDIA) ou fausse (tests)
 public:
  virtual ~IStageFactory() = default;
  virtual Result<GpuCapabilities> capabilities() = 0;
  virtual Result<std::unique_ptr<IDemuxer>> demuxer(const std::filesystem::path&) = 0;
  virtual Result<std::unique_ptr<IDecoder>> decoder(const MediaInfo&) = 0;
  virtual Result<std::unique_ptr<IInterpolator>> interpolator(InterpolationQuality) = 0;
  virtual Result<std::unique_ptr<IUpscaler>> upscaler(SuperResQuality) = 0;
  virtual Result<std::unique_ptr<IEncoder>> encoder(VideoCodec) = 0;
  virtual Result<std::unique_ptr<IMuxer>> muxer() = 0;
  virtual Result<VramInfo> vram() = 0;
};
}
```

**Fakes pour la CI** (`tests/unit/pipeline/fakes/`) : `FakeDemuxer` (paquets synthétiques, VFR, coupures de plan, flux audio/sous-titres factices, erreurs injectées à l'image N), `FakeDecoder` (images CPU numérotées), `FakeInterpolator` (enregistre les couples (a, b, t) demandés), `FakeUpscaler` (plus proche voisin), `FakeEncoder` (paquets = numéros d'image), `FakeMuxer` (journal en mémoire), `FakeStageFactory` (VRAM libre simulable, AV1 absent simulable), horloge injectable. On teste en CI : ordre et nombre d'images, cadence 24/25/30/40/50/60 i/s et VFR, coupures de plan, back-pressure, pause/reprise/annulation à tout moment, propagation d'erreurs, VRAM insuffisante, ETA, règle MP4/MKV, absence de fuite (compteur d'objets vivants dans les pools).

### 3.3 Chaîne d'étages et logique pure

- `Pipeline` construit la chaîne `[Interpolation?] → [SR?] → [HDR?] → [Vers NVENC]` à partir de `OutputPlan` : une `std::vector<std::unique_ptr<IFrameProcessor>>` exécutée dans T2, dans un même flux CUDA (ordre garanti sans événement). Ajouter HDR10 = insérer un étage ; changer l'ordre interpolation/SR = permuter deux éléments.
- `CadencePlanner` (pur) : à partir des `pts` réels de la source (VFR compris), produit pour chaque image de sortie à `k/60 s` le couple d'images sources encadrantes et la position `t ∈ [0,1]` ; `t` à moins d'un demi-millième de 0 ou 1 → copie de l'image (pas d'inférence) ; coupure de plan entre a et b → duplication de l'image la plus proche (critère ENH-012). Durée conservée à ±1 image. Pas de pulldown.
- Détection de coupure : noyau CUDA sur une vignette 160×90 de luminance (histogramme + SAD), seuil réglé sur les clips QA ; calcul dans T1, résultat porté par `FrameFlags::SceneCut`.
- Source ≥ 3840×2160 (ou déjà ≥ la cible) : pas de SR, message (ENH-009) ; source ≥ 50 i/s : pas d'interpolation, horodatages conservés (VFR conservé).

### 3.4 Threads, flux CUDA, files bornées, back-pressure

| Thread | Travail | Flux CUDA | File de sortie (capacité **[H]**) |
|---|---|---|---|
| T0 Démux | `av_read_frame`, aiguillage vidéo / autres pistes | — | paquets vidéo : 64 ; paquets copiés : 256 |
| T1 Décodage | `avcodec_send_packet/receive_frame`, copie GPU→GPU dans le pool source, conversion pivot, rotation, désentrelacement, vignette de coupure | `S_dec` | images source : **8** (= pool source) |
| T2 Traitement | chaîne d'étages | `S_proc` | images 4K : **4** (= pool 4K, surfaces pré-enregistrées NVENC) |
| T3 Encodage | `nvEncEncodePicture` asynchrone, attente des événements de fin, `nvEncLockBitstream` | flux d'E/S NVENC | paquets vidéo : 64 |
| T4 Mux | entrelacement, écriture disque | — | — |
| T5 Rapporteur | agrège compteurs atomiques, calcule i/s et ETA, appelle l'observateur | — | — |

- **Back-pressure** : chaque image occupe une surface d'un **pool de taille fixe** alloué au démarrage du job (aucune allocation dans la boucle chaude). Un étage qui veut une surface et n'en trouve pas attend qu'une surface revienne (destructeur de `PooledSurface`). Les files `BoundedQueue<T>` (mutex + `std::condition_variable_any` + `std::stop_token`) bloquent l'amont quand elles sont pleines. Mémoire bornée quelle que soit la longueur du fichier.
- **Pas de synchronisation CPU dans la boucle** : chaque `VideoFrame` porte un `GpuFence` enregistré sur le flux producteur ; le consommateur fait `cuStreamWaitEvent`. Retour d'une surface au pool : un événement « dernier usage » est enregistré, le pool ne la redonne qu'une fois l'événement passé (`cuEventQuery`, sans blocage). Seul T3 attend la fin d'encodage (événement Win32 de NVENC), ce qui est son rôle.
- **Mémoire épinglée** : seulement pour le repli logiciel (envoi des images décodées par le CPU, `cuMemHostAlloc` + `cuMemcpy2DAsync`, 3 tampons tournants) et pour l'aperçu/avant-après (§8).
- **Pause** : `PauseGate` vérifiée par T0 avant chaque paquet et par T1–T3 entre deux images ; les étages finissent l'image en cours puis attendent. État `Paused` publié quand tous les étages sont arrêtés (< 2 s, en pratique quelques dizaines de ms **[H]**).
- **Annulation** : `std::stop_source` partagé ; toutes les attentes (files, pools, pause) sont interruptibles par `std::stop_token` ; les étages sortent sans vidanger l'encodeur ; le muxer fait `abort()` (fermeture du fichier, **sans suppression** : ADR-049) ; destruction de tous les objets RAII dans l'ordre inverse ; `cuCtxSynchronize` une seule fois avant la libération ; puis `onFinished(Cancelled)`. Cible < 3 s (ENH-021). « Rétablir » (5 s) est géré par l'application (ADR-004) : elle appelle `cancel()` tout de suite (VRAM libérée), supprime le temporaire à l'expiration du délai, et « Rétablir » relance le job depuis le début.
- **Erreur dans un étage** : l'étage publie son `PipelineError` puis déclenche le `stop_source` ; la première erreur gagne, les suivantes (souvent des conséquences) vont au journal seulement.
- **Un seul job à la fois** (budget VRAM) : le moteur refuse un second `start()` (`PipelineErrorCode::Busy`). Les sessions NVENC grand public sont limitées (8 simultanées, à vérifier) : sans effet ici.

### 3.5 Encapsulation RAII des API C

Modèle commun : `template <class T, auto Deleter> using Handle = std::unique_ptr<T, FnDeleter<Deleter>>` pour les pointeurs, et une classe `UniqueValue<T, Deleter>` (déplaçable, non copiable) pour les handles non pointeurs.

| Ressource | Création → libération | Classe |
|---|---|---|
| Contexte CUDA primaire | `cuDevicePrimaryCtxRetain` → `cuDevicePrimaryCtxRelease` | `CudaContext` |
| Flux, événements | `cuStreamCreateWithPriority` / `cuEventCreate(CU_EVENT_DISABLE_TIMING)` → `Destroy` | `CudaStream`, `CudaEvent` |
| Mémoire GPU / épinglée | `cuMemAllocPitch` → `cuMemFree` ; `cuMemHostAlloc` → `cuMemFreeHost` | `DeviceBuffer`, `PinnedBuffer`, `SurfacePool` |
| FFmpeg | `avformat_open_input` → `avformat_close_input` ; `avformat_alloc_output_context2` + `avio_open` → `avio_closep` + `avformat_free_context` ; `avcodec_alloc_context3` → `avcodec_free_context` ; `av_packet_alloc`/`av_frame_alloc` → `_free` ; `av_hwdevice_ctx_create` → `av_buffer_unref` | `AvInput`, `AvOutput`, `AvCodecCtx`, `AvPacket`, `AvFrame`, `AvBuffer` |
| NVENC | `nvEncOpenEncodeSessionEx` → `nvEncDestroyEncoder` ; `nvEncRegisterResource` → `nvEncUnregisterResource` ; `nvEncCreateBitstreamBuffer` → `nvEncDestroyBitstreamBuffer` ; événements Win32 `CreateEventW` → `CloseHandle` | `NvencSession`, `NvencRegistration`, `NvencBitstream`, `WinEvent` |
| TensorRT 10 | `createInferRuntime` / `deserializeCudaEngine` / `createExecutionContext` → `delete` (interface virtuelle) | `std::unique_ptr` direct |
| RTX Video SDK | `NVSDK_NGX_CUDA_Init` + `GetCapabilityParameters` → `NVSDK_NGX_CUDA_DestroyParameters` + `NVSDK_NGX_CUDA_Shutdown` ; `NGX_CUDA_CREATE_VSR` / `NGX_CUDA_CREATE_TRUEHDR` → `NVSDK_NGX_CUDA_ReleaseFeature` (`nvsdk_ngx.h`, `nvsdk_ngx_helpers_vsr.h`). **Le tout dans une DLL pont sans `cudart`** (bloquant B4 de `spike.md` : `nvsdk_ngx_s.lib` compilée en CUDA 11.8, incompatible avec `cudaDeviceProp` de CUDA 13) | `NgxRuntime`, `RtxVideoEffect` (dans `enh_rtxvideo.dll`) |
| FRUC | `NvOFFRUCCreate` → `NvOFFRUCDestroy` ; `NvOFFRUCRegisterResource` → `NvOFFRUCUnregisterResource` (`NvOFFRUC.h`) ; DLL chargée par `LoadLibraryExW` depuis le dossier de l'app | `FrucSession` |
| DLL | `LoadLibraryExW` → `FreeLibrary` | `DynamicLibrary` |

Ordre de destruction garanti par l'ordre de déclaration des membres (inverse de la construction) ; le contexte CUDA est déclaré en premier, donc détruit en dernier.

---
