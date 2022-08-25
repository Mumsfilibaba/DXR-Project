#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformString.h"
    typedef FWindowsPlatformString FPlatformString;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformString.h"
    typedef FMacPlatformString FPlatformString;
#else
    #include "Core/Generic/GenericPlatformString.h"
    typedef FGenericPlatformString FPlatformString;
#endif