#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformMath.h"
    typedef FWindowsPlatformMath FPlatformMath;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformMath.h"
    typedef FMacPlatformMath FPlatformMath;
#else
    #include "Core/Generic/GenericPlatformMath.h"
    typedef FGenericPlatformMath FPlatformMath;
#endif
