#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformMisc.h"
    typedef CWindowsPlatformMisc PlatformMisc;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformMisc.h"
    typedef CMacPlatformMisc PlatformMisc;
#else
    #include "Core/Generic/GenericPlatformMisc.h"
    typedef CGenericPlatformMisc PlatformMisc;
#endif