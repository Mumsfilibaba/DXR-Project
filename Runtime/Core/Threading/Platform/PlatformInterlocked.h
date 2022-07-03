#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsInterlocked.h"
    typedef FWindowsInterlocked FPlatformInterlocked;
#elif PLATFORM_MACOS
    #include "Core/Threading/Mac/MacInterlocked.h"
    typedef FMacInterlocked FPlatformInterlocked;
#else
    #include "Core/Threading/Generic/GenericInterlocked.h"
    typedef FGenericInterlocked FPlatformInterlocked;
#endif
