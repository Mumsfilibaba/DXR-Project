#pragma once
#include "Core/Core.h"

#if MONOLITHIC_BUILD
    #define RENDERERCORE_API
#else
    #if RENDERERCORE_IMPL
        #define RENDERERCORE_API MODULE_EXPORT
    #else
        #define RENDERERCORE_API MODULE_IMPORT
    #endif
#endif