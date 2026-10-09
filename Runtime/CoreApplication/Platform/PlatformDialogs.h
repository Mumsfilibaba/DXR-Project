#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsPlatformDialogs.h"
    typedef FWindowsPlatformDialogs FPlatformDialogs;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacPlatformDialogs.h"
    typedef FMacPlatformDialogs FPlatformDialogs;
#else
    #include "CoreApplication/PlatformInterface/IPlatformDialogs.h"
    typedef IPlatformDialogs FPlatformDialogs;
#endif
