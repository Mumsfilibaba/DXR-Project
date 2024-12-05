#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformTypes.h"
    typedef FWindowsPlatformTypes FPlatformTypes;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformTypes.h"
    typedef FMacPlatformTypes FPlatformTypes;
#else
    #include "Core/Generic/GenericPlatformTypes.h"
    typedef FGenericPlatformTypes FPlatformTypes;
#endif