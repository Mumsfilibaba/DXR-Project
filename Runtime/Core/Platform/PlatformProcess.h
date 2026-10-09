#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformProcess.h"
    typedef FWindowsPlatformProcess FPlatformProcess;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformProcess.h"
    typedef FMacPlatformProcess FPlatformProcess;
#else
    #include "Core/PlatformInterface/IPlatformProcess.h"
    typedef IPlatformProcess FPlatformProcess;
#endif
