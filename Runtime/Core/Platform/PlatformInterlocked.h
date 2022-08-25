#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsInterlocked.h"
    typedef FWindowsInterlocked FPlatformInterlocked;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacInterlocked.h"
    typedef FMacInterlocked FPlatformInterlocked;
#else
    #include "Core/Generic/GenericInterlocked.h"
    typedef FGenericInterlocked FPlatformInterlocked;
#endif
