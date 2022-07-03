#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Time/Windows/WindowsTime.h"
    typedef FWindowsTime FPlatformTime;
#elif PLATFORM_MACOS
    #include "Core/Time/Mac/MacTime.h"
    typedef FMacTime FPlatformTime;
#else
    #include "Core/Time/Generic/GenericTime.h"
    typedef FGenericTime FPlatformTime;
#endif
