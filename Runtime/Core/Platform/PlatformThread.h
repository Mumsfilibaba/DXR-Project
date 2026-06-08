#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformThread.h"
    typedef FWindowsPlatformThread FPlatformThread;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformThread.h"
    typedef FMacPlatformThread FPlatformThread;
#else
    #include "Core/Generic/GenericPlatformThread.h"
    typedef FGenericPlatformThread FPlatformThread;
#endif
