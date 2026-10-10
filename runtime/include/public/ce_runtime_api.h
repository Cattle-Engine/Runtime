#ifndef CE_RUNTIME_API_H
#define CE_RUNTIME_API_H

#if defined(_WIN32)
    #if defined(CE_BUILDING_LIBRARY)
        #define CE_EXPORT __declspec(dllexport)
    #else
        #define CE_EXPORT __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define CE_EXPORT __attribute__((visibility("default")))
#else
    #define CE_EXPORT
#endif

#include <stdbool.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct ce_engine_t ce_engine_t;

typedef struct ce_engine_arguments_t {
    bool output_debug_as_info;
    const char* output_debug_as_info_path;
    bool debug_video;
} ce_engine_arguments_t;

CE_EXPORT void CE_ParseProgramArguments(
    int argc,
    const char* const* argv,
    ce_engine_arguments_t* output
);

CE_EXPORT ce_engine_t* CE_CreateEngine(const char* data_file, ce_engine_arguments_t args, bool debug_video);
CE_EXPORT bool CE_DestroyEngine(ce_engine_t* engine);

/**
    @brief Creates an instance

    @param engine Pointer to an engine instance (cannot be NULL)
    @param name Name for the instance (cannot be NULL)
    @param debug Enable debugging utlities on the instance
    @param data_file Optional data file
*/
CE_EXPORT bool CE_EngineCreateInstance(ce_engine_t* engine, const char* name, bool debug, const char* data_file);

/**
    @brief Destroys an instance

    @param engine Pointer to an engine instance (cannot be NULL)
    @param name Name of the instance to destroy (cannot be NULL)
*/
CE_EXPORT bool CE_EngineDestroyInstance(ce_engine_t* engine, const char* name);

/**
    @brief Runs 1 instance frame

    @param engine Pointer to an engine instance (cannot be NULL)
    @param name Name of the instance to update (cannot be NULL)
*/
CE_EXPORT int CE_EngineUpdateInstance(ce_engine_t* engine, const char* name);

/**
    @brief Updates all instances until they all quit
*/
CE_EXPORT int CE_EngineRun(ce_engine_t* engine);

#ifdef __cplusplus
    }
#endif

#endif