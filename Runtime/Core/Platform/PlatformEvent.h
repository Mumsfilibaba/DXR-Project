#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformEvent.h"
    typedef FWindowsPlatformEvent FPlatformEvent;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformEvent.h"
    typedef FMacPlatformEvent FPlatformEvent;
#else
    #include "Core/PlatformInterface/IPlatformEvent.h"
    typedef IPlatformEvent FPlatformEvent;
#endif
