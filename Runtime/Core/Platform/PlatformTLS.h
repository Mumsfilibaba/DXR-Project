#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformTLS.h"
    typedef FWindowsPlatformTLS FPlatformTLS;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformTLS.h"
    typedef FMacPlatformTLS FPlatformTLS;
#else
    #include "Core/Generic/GenericPlatformTLS.h"
    typedef FGenericPlatformTLS FPlatformTLS;
#endif
