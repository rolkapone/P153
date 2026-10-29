> Extrait de l'ancien `docs/gpu/architecture-pipeline.md` (ménage docs du 2026-10-29), texte inchangé. Sommaire : [architecture-pipeline.md](architecture-pipeline.md).

## 6. Contrat avec l'application (API publique `src/pipeline/include/enh/pipeline/`)

Le developer n'inclut **que** ces en-têtes (cible `enh_pipeline_api`) et `enh/expected.hpp` (cible `enh_common`). Aucun type CUDA, FFmpeg, Qt ou NVIDIA n'y apparaît : l'interface compile et se teste sans GPU. Le gpu-engineer reste propriétaire de ces en-têtes ; toute évolution passe par une revue commune (developer + gpu-engineer).

### 6.0 Écarts v0.1 ↔ `docs/architecture/interface-pipeline.md` (ex-§ 12), tranchés

| # | Écart | Décision | ADR |
|---|---|---|---|
| 1 | `enh::Result<T>` (pipeline) vs `enh::Expected` (application) | **`enh::Expected<T, PipelineError>`** partout dans l'API ; alias unique dans `enh_common` | ADR-003, ADR-047 |
| 2 | Progression ≤ 4/s vs ≤ 10 Hz | **4 fois/s (250 ms)**, plafond de 10 Hz respecté ; étapes, états et fin émis immédiatement ; ETA calculée par le cœur | ADR-051 |
| 3 | `.enhance-part` géré par le pipeline vs ADR-010 | **L'application possède nommage, temporaire, renommage, suppression, nettoyage** ; le pipeline crée en `CREATE_NEW` le chemin exact reçu, écrit, vide, ferme | ADR-010, ADR-049 |
| 4 | `probe`/`plan` dans le moteur vs `IMediaProbe` + `plan` pur | `IMediaProbe::probe` / `thumbnail` CPU seulement (< 1 s) ; **`plan()` fonction libre pure** (`MediaInfo`, `PipelineParams`, `GpuCapabilities`) | ADR-051 |
| 5 | `unique_ptr<IJob>` + `shared_ptr<IJobObserver>` vs `JobHandle` RAII | **`JobHandle`** valeur déplaçable ; observateur par **référence** ; rappels `noexcept`, un seul thread du pipeline, sérialisés, < 1 ms, **aucun appel en retour** | ADR-051 |
| 6 | `ErrorCode` technique vs P-08 | **`PipelineErrorCode` calqué sur P-08** (une ligne = un code) + codes hors P-08 rangés explicitement | ADR-047, § 6.6 |
| 7 | Chemins longs / Unicode vers FFmpeg | **`AVIOContext` Win32 maison** (`CreateFileW` + `\\?\`), pas de chemin UTF-8 donné à FFmpeg ; ouvertures secondaires refusées | ADR-049 |
| 8 | Numérotation P01–P11 | **ADR-040 à ADR-050**, + ADR-051 (contrat) | — |

Noms retenus (ceux de l'application) : `IMediaProbe`, `IPipeline`, `PipelineError`, `PipelineParams`, `JobSpec`, `JobHandle`, `IJobObserver`, `FakePipeline`. Les noms v0.1 `IPipelineEngine`, `PresetSpec`, `IJob`, `Error`, `createEngine`, `createFakeEngine` disparaissent.

### 6.1 Types communs

```cpp
// src/common/include/enh/expected.hpp  (cible enh_common, en-têtes seuls, partagée app + pipeline)
#include <tl/expected.hpp>
namespace enh {
template <class T, class E> using Expected = tl::expected<T, E>;   // std::expected le jour du C++23
template <class E> using Unexpected = tl::unexpected<E>;
}

// src/pipeline/include/enh/pipeline/types.hpp
namespace enh::pipeline {
struct Size { std::uint32_t width = 0, height = 0; };                  // pixels
struct Rational { std::int32_t num = 0, den = 1; };                    // ex. 30000/1001
enum class Container : std::uint8_t { Mp4, Mkv };
enum class TrackKind : std::uint8_t { Audio, Subtitle, Attachment, Data };
}
```

### 6.2 Paramètres et description d'un job (sérialisables)

```cpp
namespace enh::pipeline {
enum class VideoCodec : std::uint8_t { Hevc, Av1 };
enum class BitDepth : std::uint8_t { Bits8 = 8, Bits10 = 10 };
enum class ContainerPolicy : std::uint8_t { Mp4WithMkvFallback, ForceMkv };   // ForceMkv : options avancées (ENH-035)
enum class SuperResQuality : std::uint8_t { Medium, High, Highest };          // correspondance SDK interne
enum class InterpolationQuality : std::uint8_t { Full, Reduced };             // Reduced : « Rapide » seulement (PO n° 5)
enum class DynamicRange : std::uint8_t { Sdr, Hdr10 };                        // Hdr10 : ENH-032, refusé au MVP
enum class RateControl : std::uint8_t { Vbr, ConstQuality };

struct EncodeSettings {
  VideoCodec codec = VideoCodec::Hevc;
  BitDepth depth = BitDepth::Bits10;
  RateControl rate_control = RateControl::Vbr;
  std::uint32_t target_kbps = 40'000;     // pour 3840×2160 à 60 i/s, ajusté au prorata (§ 4)
  std::uint32_t max_kbps = 60'000;
  std::uint8_t nvenc_preset = 5;          // 1..7
  std::uint8_t lookahead_frames = 16;
  bool multipass = true;
};

// Produit par l'application à partir d'un préréglage (fonction pure de l'ADR-005).
struct PipelineParams {
  SuperResQuality super_res = SuperResQuality::High;
  InterpolationQuality interpolation = InterpolationQuality::Full;
  bool interpolate_below_50fps = true;    // décision PO n° 1 : < 50 i/s → 60 i/s, jamais au-delà
  Size target_box_px{3840, 2160};         // boîte englobante, rapport d'aspect conservé (ENH-035)
  DynamicRange range = DynamicRange::Sdr;
  EncodeSettings encode;
  ContainerPolicy container = ContainerPolicy::Mp4WithMkvFallback;
};

struct TimeRange { std::chrono::duration<double> start, duration; };   // secondes

struct JobSpec {
  std::uint32_t schema_version = 1;
  std::filesystem::path input;            // source
  std::filesystem::path output_path;      // chemin EXACT à créer (le temporaire choisi par l'application, ADR-010)
  Container container = Container::Mp4;   // recopié de OutputPlan::container (vérifié par start())
  PipelineParams params;
  std::optional<TimeRange> range;         // aperçu de quelques secondes (ENH-036)
};
}
```
La sérialisation JSON appartient à l'application ; tous les champs sont des types simples et `schema_version` permet de relire une file persistée (ENH-022) ou un journal de reprise (ENH-038).

### 6.3 Avant de lancer : sondage, vignette, plan (sans GPU)

```cpp
namespace enh::pipeline {
struct TrackInfo {
  int index; TrackKind kind; std::string codec; std::string language; std::string title;
  bool mp4_compatible;                     // règle de l'ADR-050
};
struct ColorDescription { /* matrice, primaires, transfert, plage — énumérations du pipeline */ };
struct MediaInfo {
  std::chrono::duration<double> duration;  // s
  Size size_px;                            // taille d'AFFICHAGE (rotation et SAR appliqués)
  Size coded_size_px; Rational sample_aspect; int rotation_deg;
  Rational avg_fps; bool variable_fps;
  std::uint64_t frame_count_estimate;
  std::string video_codec; int bit_depth; ColorDescription color; bool interlaced; bool hdr10;
  std::vector<TrackInfo> tracks;           // audio, sous-titres, pièces jointes
};
struct Thumbnail { Size size_px; std::vector<std::byte> rgba; };   // RGBA 8 bits non prémultiplié, 4 × largeur octets par ligne

class IMediaProbe {                        // CPU seulement ; const, appelable depuis plusieurs threads à la fois
 public:
  virtual ~IMediaProbe() = default;
  virtual Expected<MediaInfo, PipelineError> probe(const std::filesystem::path& input,
                                                   std::stop_token st = {}) const noexcept = 0;   // < 1 s
  virtual Expected<Thumbnail, PipelineError> thumbnail(const std::filesystem::path& input, Size max_px,
                                                       std::stop_token st = {}) const noexcept = 0;
};

enum class PlanNote : std::uint8_t {
  NoUpscaleAlreadyAtTarget, NoInterpolationAtOrAbove50, SoftwareDecode, MkvFallback,
  SubtitleConvertedToMovText, SourceHdr10Kept, Deinterlaced };
struct OutputPlan {
  Size output_px; Rational output_fps; bool upscale; bool interpolate; bool hw_decode;
  Container container; std::string_view extension;   // ".mp4" | ".mkv" (l'application construit les noms)
  std::vector<int> mkv_reason_tracks;                 // pistes qui imposent le MKV (message avant traitement)
  std::uint64_t frames_out_estimate;
  std::uint64_t size_estimate_bytes;                  // prévérification d'espace disque (ENH-024)
  std::uint32_t vram_estimate_mib;                    // à comparer à GpuCapabilities::vram_free_mib
  std::chrono::seconds duration_estimate;             // [H] tables de vitesse mesurées (QA), indicatif
  std::vector<PlanNote> notes;
};

// Fonction PURE : ni E/S, ni GPU, ni état global ; mêmes entrées → même sortie (testée en CI).
Expected<OutputPlan, PipelineError> plan(const MediaInfo& media, const PipelineParams& params,
                                         const GpuCapabilities& caps) noexcept;

std::unique_ptr<IMediaProbe> createMediaProbe();     // implémentation FFmpeg (CPU)
}
```
Règles de `plan()` : interpolation si `avg_fps < 50` (VFR : cadence moyenne) → 60 i/s constants, sinon cadence et horodatages conservés ; super-résolution seulement si le facteur `min(boîte.l / l, boîte.h / h)` est > 1 (sinon `NoUpscaleAlreadyAtTarget`, taille conservée) ; conteneur par l'ADR-050 ; `hw_decode` d'après `caps` (codecs NVDEC) ; erreurs possibles : `UnsupportedFormat`, `ComponentMissing` (ex. AV1 absent), `InvalidParams` (ex. HDR10 au MVP, `Reduced` hors « Rapide » détecté par l'application en amont).
`probe` : limites `probesize` / `analyzeduration` réglées et rappel d'interruption FFmpeg (`AVIOInterruptCB`) avec échéance de 2 s et `stop_token` → jamais bloqué sur un fichier piégé ; `thumbnail` : décodage logiciel d'une image clé vers 10 % de la durée, mise à l'échelle `libswscale` (LGPL), rotation appliquée.

### 6.4 Lancer et piloter un job

```cpp
namespace enh::pipeline {
struct GpuCapabilities {
  std::string gpu_name; std::string driver_version; int compute_major = 0, compute_minor = 0;
  std::uint32_t vram_total_mib = 0, vram_free_mib = 0;
  bool nvenc_hevc_10bit = false, nvenc_av1 = false, nvenc_av1_10bit = false;
  std::vector<std::string> nvdec_codecs;     // codecs décodables par NVDEC
  bool rtx_video_sdk = false, tensorrt = false, interpolation_model = false;
};

enum class JobStage : std::uint8_t { Opening, PreparingModels, Processing, Finalizing };
enum class JobRunState : std::uint8_t { Running, Paused };

class IJobObserver {               // implémenté par l'application ; DOIT survivre au JobHandle
 public:
  virtual ~IJobObserver() = default;
  virtual void onStageChanged(JobStage stage) noexcept = 0;
  virtual void onStateChanged(JobRunState state) noexcept = 0;      // Paused quand effectif (< 2 s)
  virtual void onProgress(const Progress& p) noexcept = 0;          // 4 fois/s
  virtual void onFinished(Expected<JobResult, PipelineError> r) noexcept = 0;  // exactement une fois, en dernier
};

class JobHandle {                  // déplaçable, non copiable
 public:
  JobHandle() noexcept = default;
  JobHandle(JobHandle&&) noexcept; JobHandle& operator=(JobHandle&&) noexcept;
  ~JobHandle();                    // job encore actif → cancel() + attente (≤ 3 s) ; après onFinished → immédiat
  void pause() noexcept;           // non bloquants, idempotents, appelables depuis n'importe quel thread
  void resume() noexcept;          //   (sauf depuis un rappel de l'observateur, voir § 6.7)
  void cancel() noexcept;          // → onFinished(PipelineError{Cancelled}) (< 3 s, VRAM libérée, fichier fermé)
  void wait() noexcept;            // bloque jusqu'au retour de onFinished (tests, fermeture de l'application)
  [[nodiscard]] bool valid() const noexcept;
 private:
  std::unique_ptr<detail::IJobControl> impl_;   // réel ou faux
};

class IPipeline {
 public:
  virtual ~IPipeline() = default;
  virtual Expected<GpuCapabilities, PipelineError> capabilities() noexcept = 0;  // < 1 s, hors thread UI (ENH-011, ENH-031)
  virtual Expected<JobHandle, PipelineError> start(const JobSpec& spec, IJobObserver& obs) noexcept = 0;  // < 50 ms
};

Expected<std::unique_ptr<IPipeline>, PipelineError> createNvidiaPipeline(const PipelineOptions& opt) noexcept;
}
// Pour les tests (cible enh_pipeline_fake) : FakePipeline, FakeMediaProbe (§ 6.8).
```
`start()` ne fait que valider (`Busy`, `InvalidParams`, cohérence `container` / plan) et lancer les threads ; ouverture des fichiers, contrôle de VRAM, construction des modèles TensorRT (`PreparingModels`, 1–3 min au premier lancement **[H]**) se font ensuite et leurs erreurs arrivent par `onFinished`. Si `start()` renvoie une erreur, aucun rappel n'est émis. Un seul job actif par `IPipeline`.

Étapes vues par l'interface (*Lecture · Agrandissement · Fluidité · Enregistrement*, P-05) : traduites **dans le cœur** (fonction pure) à partir de `JobStage` et de l'`OutputPlan` : `Opening` → Lecture ; `Processing` → Agrandissement si `upscale`, sinon Fluidité si `interpolate`, sinon Enregistrement ; `Finalizing` → Enregistrement ; `PreparingModels` → libellé à fournir par l'ux-designer. Les étages tournent en parallèle : une étape « dominante » est plus honnête qu'une fausse séquence.

### 6.5 Progression et résultat

```cpp
namespace enh::pipeline {
struct StageRates { double decode_fps, interpolate_fps, upscale_fps, encode_fps; };   // journal, QA
struct Progress {
  std::uint64_t frames_done;                // images de SORTIE écrites
  std::uint64_t frames_total;               // estimation (durée × cadence de sortie), affinée en cours de route
  double fps;                               // images de sortie / s sur la dernière période (≈ instantané) ; le lissage et l'ETA sont faits par le cœur (EtaEstimator)
  std::chrono::duration<double> elapsed;    // depuis start(), pauses exclues
  std::uint32_t vram_used_mib;              // VRAM du job (cuMemGetInfo, base mesurée avant le job)
  StageRates stage_fps;
};
enum class JobWarning : std::uint8_t { SoftwareDecodeFallback, CorruptFramesConcealed, SubtitleConvertedToMovText, TrackSkipped };
struct JobResult {
  std::uint64_t frames_out; Size output_px; Rational output_fps; Container container;
  std::uint64_t bytes_written;
  std::chrono::duration<double> elapsed; double average_fps; std::uint32_t vram_peak_mib;
  std::vector<JobWarning> warnings;         // aussi au journal ; aucune interruption pour un avertissement
};
}
```

### 6.6 Erreurs (alignées sur P-08)

```cpp
namespace enh::pipeline {
enum class PipelineErrorCode : std::uint16_t {
  // Une ligne du tableau P-08 = un code
  SourceCorrupt,      // « endommagé à partir de 12 min 40 s »     → at
  UnsupportedFormat,  // « format vidéo non pris en charge »        (pas de piste vidéo, codec, 4:4:4 12 bits…)
  OutOfVram,          // « pas assez de mémoire »                  → required_mib, available_mib
  DiskFull,           // « le disque D: est plein » (pendant)       (« avant » = prévérification de l'application)
  DeviceLost,         // « la carte graphique ne répond plus »      (CUDA_ERROR_LAUNCH_FAILED, NV_ENC_ERR_DEVICE_NOT_EXIST, TDR…)
  WriteDenied,        // « ne peut pas écrire dans ce dossier »
  SourceMissing,      // « le fichier d'origine n'est plus là »
  Internal,           // « un problème inattendu »                  + Copier le détail / Afficher les journaux
  // Hors P-08
  Cancelled,          // pas un échec : état « Annulé »
  SourceUnreadable,   // fichier présent mais illisible (verrouillé, droits) : ligne à ajouter à P-08 (ux-designer)
  ComponentMissing,   // → component ; bandeau des prérequis (ENH-031), pas une erreur de carte
  OutputExists,       // l'application choisit un autre nom et relance (jamais affiché)
  InvalidParams,      // défaut de validation côté application → affiché comme Internal
  Busy,               // un job déjà actif → défaut d'orchestration → affiché comme Internal
};
enum class Component : std::uint8_t { None, NvidiaGpu, Driver, NvencHevc, NvencAv1, Nvenc10Bit, RtxVideoSdk, TensorRt, InterpolationModel, RtxVideoFeatureUntrusted /* ENH-040 : DLL de fonction VSR non livree par Enhance */ };

struct PipelineError {
  PipelineErrorCode code = PipelineErrorCode::Internal;
  std::optional<std::chrono::duration<double>> at;       // SourceCorrupt : instant dans la source (s)
  std::uint32_t required_mib = 0, available_mib = 0;     // OutOfVram
  Component component = Component::None;                 // ComponentMissing
  std::string detail;                                    // anglais, technique : journal et « Copier le détail »
  std::int64_t native_code = 0;                          // CUresult, NVENCSTATUS, AVERROR, GetLastError…
};
}
```
Le pipeline ne produit **aucune phrase** ; la présentation traduit le code et ses données en français (ADR-003, ADR-008). Un fichier corrompu **isolé** (quelques images illisibles) n'est pas une erreur : images masquées, `CorruptFramesConcealed` dans `JobResult::warnings` ; `SourceCorrupt` est réservé à un flux qui ne peut plus être lu (seuil à fixer avec QA/secdevops dans ENH-015).

### 6.7 Règles de threads et de durée de vie (fermes)

1. Tous les rappels de `IJobObserver` viennent d'**un seul thread du pipeline** (rapporteur T5), jamais d'un thread d'étage ni du thread UI ; ils sont **sérialisés** ; ordre garanti : `onStageChanged(Opening)` … `onFinished` (dernier, exactement une fois, y compris après annulation ou erreur).
2. Rappels **`noexcept`** et **non bloquants** (< 1 ms) : côté application, copie de la donnée + `IExecutor::post` (ADR-002) ; pas d'E/S, pas de verrou disputé.
3. **Aucun appel en retour** vers le pipeline depuis un rappel (`pause`, `resume`, `cancel`, `wait`, destruction du `JobHandle`, `start`) : l'application reposte d'abord vers son thread. Destruction et `wait()` depuis un rappel = interblocage → assertion en debug.
4. L'observateur doit vivre plus longtemps que le `JobHandle` ; le destructeur du `JobHandle` attend que le dernier rappel soit rendu.
5. `pause`, `resume`, `cancel` sont thread-safe et non bloquants ; leurs effets arrivent par `onStateChanged(Paused | Running)` et `onFinished`.
6. Détruire le `JobHandle` après `onFinished` est immédiat (< 10 ms) ; avant `onFinished`, il annule et attend (≤ 3 s) — à réserver à la fermeture de l'application, hors thread UI si possible.
7. `IMediaProbe` est sans état partagé : plusieurs `probe`/`thumbnail` en parallèle sont permis (pool de l'application, ADR-002) ; interrompables par `stop_token`.
8. Le journal passe par la façade `enh::log` (ADR-007) ; `av_log` est redirigé une seule fois (`av_log_set_callback`) à la création de la sonde ou du pipeline.

### 6.8 Faux pour les tests de l'application (CI, sans GPU)

`FakePipeline` (cible `enh_pipeline_fake`) : scénario déterministe configurable — vitesse (i/s), nombre d'images, étapes, durée de `PreparingModels`, erreur injectée à l'image N (n'importe quel `PipelineErrorCode`), délai de pause / d'annulation, capacités (AV1 absent, VRAM libre) ; horloge **pilotée par le test** (pas de `sleep`) ; un mode « même thread » où les rappels sont émis par un appel `step()` du test. `FakeMediaProbe` : table chemin → `MediaInfo` / erreur, vignette unie. Ces faux respectent les mêmes règles que le réel (§ 6.7) et sont vérifiés par la même suite de tests de contrat (`tests/unit/pipeline/contract/`), exécutée aussi contre le vrai pipeline dans `tests/gpu/`.

---
