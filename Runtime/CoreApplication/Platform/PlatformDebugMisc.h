#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsDebugMisc.h"
    typedef CWindowsDebugMisc PlatformDebugMisc;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacDebugMisc.h"
    typedef CMacDebugMisc PlatformDebugMisc;
#else
    #include "CoreApplication/Generic/GenericDebugMisc.h"
    typedef CGenericDebugMisc PlatformDebugMisc;
#endif