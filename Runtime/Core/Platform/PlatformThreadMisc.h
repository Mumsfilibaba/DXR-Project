#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsThreadMisc.h"
    typedef FWindowsThreadMisc FPlatformThreadMisc;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacThreadMisc.h"
    typedef FMacThreadMisc FPlatformThreadMisc;
#else
    #include "Core/Generic/GenericThreadMisc.h"
    typedef FGenericThreadMisc FPlatformThreadMisc;
#endif