#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformMath.h"
    typedef FWindowsPlatformMath FPlatformMath;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformMath.h"
    typedef FMacPlatformMath FPlatformMath;
#else
    #include "Core/PlatformInterface/IPlatformMath.h"
    typedef IPlatformMath FPlatformMath;
#endif
