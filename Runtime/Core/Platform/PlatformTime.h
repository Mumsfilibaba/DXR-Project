#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformTime.h"
    typedef FWindowsPlatformTime FPlatformTime;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformTime.h"
    typedef FMacPlatformTime FPlatformTime;
#else
    #include "Core/Generic/GenericPlatformTime.h"
    typedef FGenericPlatformTime FPlatformTime;
#endif
