#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformThread.h"
    typedef FWindowsPlatformThread FPlatformThread;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformThread.h"
    typedef FMacPlatformThread FPlatformThread;
#else
    #include "Core/PlatformInterface/IPlatformThread.h"
    typedef IPlatformThread FPlatformThread;
#endif
