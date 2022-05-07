#pragma once
#include "Core/Core.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// Canvas API

#if MONOLITHIC_BUILD
    #define CANVAS_API
#else
    #if CANVAS_IMPL
        #define CANVAS_API MODULE_EXPORT
    #else
        #define CANVAS_API MODULE_IMPORT
    #endif
#endif