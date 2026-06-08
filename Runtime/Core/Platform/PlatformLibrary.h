#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformLibrary.h"
    typedef FWindowsPlatformLibrary FPlatformLibrary;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformLibrary.h"
    typedef FMacPlatformLibrary FPlatformLibrary;
#else
    #include "Core/Generic/GenericPlatformLibrary.h"
    typedef FGenericPlatformLibrary FPlatformLibrary;
#endif
