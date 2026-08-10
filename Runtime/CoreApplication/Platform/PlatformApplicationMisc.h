#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsApplicationMisc.h"
    typedef FWindowsApplicationMisc FPlatformApplicationMisc;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacApplicationMisc.h"
    typedef FMacApplicationMisc FPlatformApplicationMisc;
#else
    #include "CoreApplication/PlatformInterface/IPlatformApplicationMisc.h"
    typedef IPlatformApplicationMisc FPlatformApplicationMisc;
#endif
