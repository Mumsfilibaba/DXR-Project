#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CoreApplication API

#if MONOLITHIC_BUILD
    #define COREAPPLICATION_API
#else
    #if COREAPPLICATION_IMPL
        #define COREAPPLICATION_API MODULE_EXPORT
    #else
        #define COREAPPLICATION_API MODULE_IMPORT
    #endif
#endif