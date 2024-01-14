#pragma once
#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformStackTrace.h"
    typedef FWindowsPlatformStackTrace FPlatformStackTrace;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformStackTrace.h"
    typedef FMacPlatformStackTrace FPlatformStackTrace;
#else
    #include "Core/Generic/GenericPlatformStackTrace.h"
    typedef FGenericPlatformStackTrace FPlatformStackTrace;
#endif