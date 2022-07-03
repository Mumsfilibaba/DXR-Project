#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsThreadMisc.h"
    typedef FWindowsThreadMisc FPlatformThreadMisc;
#elif PLATFORM_MACOS
    #include "Core/Threading/Mac/MacThreadMisc.h"
    typedef FMacThreadMisc FPlatformThreadMisc;
#else
    #include "Core/Threading/Generic/GenericThreadMisc.h"
    typedef FGenericThreadMisc FPlatformThreadMisc;
#endif