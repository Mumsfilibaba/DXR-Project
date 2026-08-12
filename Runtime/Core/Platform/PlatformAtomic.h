#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformAtomic.h"
    typedef FWindowsPlatformAtomic FPlatformAtomic;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformAtomic.h"
    typedef FMacPlatformAtomic FPlatformAtomic;
#else
    #include "Core/PlatformInterface/IPlatformAtomic.h"
    typedef IPlatformAtomic FPlatformAtomic;
#endif
