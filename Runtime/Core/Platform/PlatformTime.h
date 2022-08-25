#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsTime.h"
    typedef FWindowsTime FPlatformTime;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacTime.h"
    typedef FMacTime FPlatformTime;
#else
    #include "Core/Generic/GenericTime.h"
    typedef FGenericTime FPlatformTime;
#endif
