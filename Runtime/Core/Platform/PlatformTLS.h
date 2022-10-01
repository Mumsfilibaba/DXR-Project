#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsTLS.h"
    typedef FWindowsTLS FPlatformTLS;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacTLS.h"
    typedef FMacTLS FPlatformTLS;
#else
    #include "Core/Generic/GenericTLS.h"
    typedef FGenericTLS FPlatformTLS;
#endif