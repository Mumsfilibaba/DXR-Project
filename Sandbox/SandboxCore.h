#pragma once
#include <Core/Core.h>

#if MONOLITHIC_BUILD
    #define SANDBOX_API
#else
    #if SANDBOX_IMPL
        #define SANDBOX_API MODULE_EXPORT
    #else
        #define SANDBOX_API MODULE_IMPORT
    #endif
#endif