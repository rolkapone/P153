#ifndef ENH_RTXVIDEO_BRIDGE_H
#define ENH_RTXVIDEO_BRIDGE_H

/*
 * API C du pont RTX Video (enh_rtxvideo_bridge.dll ; ADR-037, docs/gpu/ENH-040.md).
 *
 * Pourquoi une DLL a part : la bibliotheque NGX du RTX Video SDK 1.1.0 (nvsdk_ngx_s.lib) a ete
 * compilee avec les en-tetes CUDA 11.8 et appelle trois fonctions du runtime CUDA. Liee au runtime
 * CUDA 13 de l'application, elle plante (0xC0000409, bloquant B4). Le pont contient NGX, ne lie
 * PAS cudart et fournit lui-meme ces trois fonctions par l'API pilote.
 *
 * Regles de la frontiere (deux CRT differents : pont /MT, application /MD) :
 * - C pur : aucun objet C++, aucune exception ne traverse ; aucune memoire allouee d'un cote n'est
 *   liberee de l'autre (les objets opaques sont detruits par la fonction *_destroy du pont).
 * - Chaque structure commence par struct_size (= sizeof vu par l'appelant) : une version future
 *   ajoute des champs a la fin sans casser l'ABI ; le pont refuse une taille inferieure a la v1.
 * - rvb_abi_version() est verifiee avant tout autre appel (RVB_ABI_VERSION).
 * - Aucun type CUDA ni NGX : contexte et flux CUDA en void*, CUtexObject / CUsurfObject en
 *   uint64_t (meme representation que dans cuda.h).
 */

#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RVB_ABI_VERSION 1u
#define RVB_CALL __cdecl

/* Code retour : NVSDK_NGX_Result tel quel (0x1 = succes, 0xBAD000xx = echec NGX) ou code du pont
 * (0xE00000xx). Utiliser rvb_succeeded(). */
typedef uint32_t rvb_result;

#define RVB_OK 0x00000001u
#define RVB_NGX_FAIL_MASK 0xFFF00000u
#define RVB_NGX_FAIL_BASE 0xBAD00000u
#define RVB_E_BASE 0xE0000000u
#define RVB_E_INVALID_ARG 0xE0000001u     /* pointeur nul, qualite hors bornes, rectangle vide */
#define RVB_E_BAD_STRUCT_SIZE 0xE0000002u /* struct_size < taille de la v1 */
#define RVB_E_ALREADY_INITIALIZED 0xE0000003u /* un seul runtime NGX par processus */
#define RVB_E_INTERNAL 0xE0000004u            /* exception C++ interceptee dans le pont */
#define RVB_E_BUSY 0xE0000005u                /* runtime detruit alors que des effets vivent */
#define RVB_E_CUDA 0xE0000006u                /* echec de l'API pilote CUDA (contexte) */

/* Succes : ni echec NGX, ni code d'erreur du pont. */
static inline int rvb_succeeded(rvb_result r) {
    return ((r & RVB_NGX_FAIL_MASK) != RVB_NGX_FAIL_BASE) && ((r & 0xF0000000u) != RVB_E_BASE);
}

typedef struct rvb_runtime rvb_runtime; /* opaque : runtime NGX (CUDA) du processus */
typedef struct rvb_effect rvb_effect;   /* opaque : un effet VSR cree sur un contexte / flux */

/* Niveaux de journal NGX transmis au rappel. RVB_LOG_OFF : aucun journal (release). */
#define RVB_LOG_OFF 0u
#define RVB_LOG_ON 1u
#define RVB_LOG_VERBOSE 2u

/* Rappel de journal : appele depuis n'importe quel thread, doit etre thread-safe et ne jamais
 * lever d'exception ; `message` n'est valide que pendant l'appel. */
typedef void(RVB_CALL* rvb_log_fn)(void* user, uint32_t level, const char* message);

typedef struct rvb_init_params {
    uint32_t struct_size;
    uint32_t log_level;       /* RVB_LOG_* ; NGX n'ecrit jamais de fichier journal lui-meme */
    const wchar_t* data_path; /* dossier de donnees NGX (existant, accessible en ecriture) */
    void* cu_context;         /* CUcontext utilise pendant l'initialisation (obligatoire) */
    rvb_log_fn log_fn;        /* facultatif */
    void* log_user;
} rvb_init_params;

typedef struct rvb_vsr_caps {
    uint32_t struct_size;
    uint32_t available;            /* VSR.Available */
    uint32_t needs_updated_driver; /* VSR.NeedsUpdatedDriver */
    uint32_t min_driver_major;
    uint32_t min_driver_minor;
    uint32_t reserved;
    uint64_t scratch_bytes; /* NVSDK_NGX_CUDA_GetScratchBufferSize (0 attendu) */
} rvb_vsr_caps;

typedef struct rvb_vsr_create_params {
    uint32_t struct_size;
    uint32_t reserved;
    void* cu_context; /* CUcontext */
    void* cu_stream;  /* CUstream : tout le travail de l'effet est mis en file sur ce flux */
} rvb_vsr_create_params;

typedef struct rvb_rect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} rvb_rect;

typedef struct rvb_vsr_eval_params {
    uint32_t struct_size;
    uint32_t quality;        /* 0 = bicubique ... 4 = Ultra (NVSDK_NGX_VSR_QualityLevel) */
    uint64_t input_texture;  /* CUtexObject : ARGB8 ou ABGR10, coordonnees normalisees */
    uint64_t output_surface; /* CUsurfObject : meme format que l'entree */
    rvb_rect input_rect;
    rvb_rect output_rect;
} rvb_vsr_eval_params;

/* Fonctions exportees (enh_rtxvideo_bridge.def). Toutes sont sans exception. */
uint32_t RVB_CALL rvb_abi_version(void);
rvb_result RVB_CALL rvb_runtime_create(const rvb_init_params* params, rvb_runtime** out);
rvb_result RVB_CALL rvb_runtime_destroy(rvb_runtime* runtime);
rvb_result RVB_CALL rvb_vsr_query(rvb_runtime* runtime, rvb_vsr_caps* out);
rvb_result RVB_CALL rvb_vsr_create(rvb_runtime* runtime, const rvb_vsr_create_params* params,
                                   rvb_effect** out);
rvb_result RVB_CALL rvb_vsr_evaluate(rvb_effect* effect, const rvb_vsr_eval_params* params);
rvb_result RVB_CALL rvb_effect_destroy(rvb_effect* effect);

/* Types des pointeurs obtenus par GetProcAddress (chargement explicite, ADR-048). */
typedef uint32_t(RVB_CALL* rvb_abi_version_fn)(void);
typedef rvb_result(RVB_CALL* rvb_runtime_create_fn)(const rvb_init_params*, rvb_runtime**);
typedef rvb_result(RVB_CALL* rvb_runtime_destroy_fn)(rvb_runtime*);
typedef rvb_result(RVB_CALL* rvb_vsr_query_fn)(rvb_runtime*, rvb_vsr_caps*);
typedef rvb_result(RVB_CALL* rvb_vsr_create_fn)(rvb_runtime*, const rvb_vsr_create_params*,
                                                rvb_effect**);
typedef rvb_result(RVB_CALL* rvb_vsr_evaluate_fn)(rvb_effect*, const rvb_vsr_eval_params*);
typedef rvb_result(RVB_CALL* rvb_effect_destroy_fn)(rvb_effect*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ENH_RTXVIDEO_BRIDGE_H */
