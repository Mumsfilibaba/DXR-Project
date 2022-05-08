#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Logging/Log.h"
#include "Core/Debug/Debug.h"

#include <Metal/Metal.h>

#if MONOLITHIC_BUILD
    #define METAL_RHI_API
#else
    #if METALRHI_IMPL
        #define METAL_RHI_API MODULE_EXPORT
    #else
        #define METAL_RHI_API MODULE_IMPORT
    #endif
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Metal Log Macros

#if !PRODUCTION_BUILD
    #define METAL_ERROR(...)                      \
        do                                        \
        {                                         \
            LOG_ERROR("[MetalRHI] " __VA_ARGS__); \
            CDebug::DebugBreak();                 \
        } while (false)
    
    #define METAL_ERROR_COND(bCondition, ...) \
        do                                    \
        {                                     \
            if (!(bCondition))                \
            {                                 \
                METAL_ERROR(__VA_ARGS__);     \
            }                                 \
        } while (false)
    
    #define METAL_WARNING(...)                      \
        do                                          \
        {                                           \
            LOG_WARNING("[MetalRHI] " __VA_ARGS__); \
        } while (false)

    #define METAL_WARNING_COND(bCondition, ...) \
        do                                      \
        {                                       \
            if (!(bCondition))                  \
            {                                   \
                METAL_WARNING(__VA_ARGS__);     \
            }                                   \
        } while (false)

    #define METAL_INFO(...)                      \
        do                                       \
        {                                        \
            LOG_INFO("[MetalRHI] " __VA_ARGS__); \
        } while (false)
#else
    #define METAL_ERROR_COND(bCondition, ...) \
        do                                    \
        {                                     \
            (void)(bCondition);               \
        } while(false)

    #define METAL_ERROR(...)   do { (void)(0); } while(false)

    #define METAL_WARNING_COND(bCondition, ...) \
        do                                      \
        {                                       \
            (void)(bCondition);                 \
        } while(false)

    #define METAL_WARNING(...) do { (void)(0); } while(false)

    #define METAL_INFO(...)    do { (void)(0); } while(false)
#endif
