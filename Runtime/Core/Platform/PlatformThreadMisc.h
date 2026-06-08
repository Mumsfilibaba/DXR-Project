#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformThreadMisc.h"
    typedef FWindowsPlatformThreadMisc FPlatformThreadMisc;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformThreadMisc.h"
    typedef FMacPlatformThreadMisc FPlatformThreadMisc;
#else
    #include "Core/Generic/GenericPlatformThreadMisc.h"
    typedef FGenericPlatformThreadMisc FPlatformThreadMisc;
#endif
