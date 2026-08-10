#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsApplication.h"
    typedef FWindowsApplication FPlatformApplication;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacApplication.h"
    typedef FMacApplication FPlatformApplication;
#else
    #include "CoreApplication/PlatformInterface/IPlatformApplication.h"
    typedef IPlatformApplication FPlatformApplication;
#endif
