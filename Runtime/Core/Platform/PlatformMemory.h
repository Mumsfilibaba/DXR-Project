#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformMemory.h"
    typedef FWindowsPlatformMemory FPlatformMemory;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformMemory.h"
    typedef FMacPlatformMemory FPlatformMemory;
#else
    #include "Core/PlatformInterface/IPlatformMemory.h"
    typedef IPlatformMemory FPlatformMemory;
#endif
