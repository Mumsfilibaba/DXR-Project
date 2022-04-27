#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Time/Windows/WindowsTime.h"
    typedef CWindowsTime PlatformTime;
#elif PLATFORM_MACOS
    #include "Core/Time/Mac/MacTime.h"
    typedef CMacTime PlatformTime;
#else
    #include "Core/Time/Generic/GenericTime.h"
    typedef CGenericTime PlatformTime;
#endif
