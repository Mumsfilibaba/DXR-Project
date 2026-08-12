#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformMisc.h"
    typedef FWindowsPlatformMisc FPlatformMisc;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformMisc.h"
    typedef FMacPlatformMisc FPlatformMisc;
#else
    #include "Core/PlatformInterface/IPlatformMisc.h"
    typedef IPlatformMisc FPlatformMisc;
#endif
