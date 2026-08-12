#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformThreadMisc.h"
    typedef FWindowsPlatformThreadMisc FPlatformThreadMisc;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformThreadMisc.h"
    typedef FMacPlatformThreadMisc FPlatformThreadMisc;
#else
    #include "Core/PlatformInterface/IPlatformThreadMisc.h"
    typedef IPlatformThreadMisc FPlatformThreadMisc;
#endif
