#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsCriticalSection.h"
    typedef FWindowsCriticalSection FCriticalSection;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacCriticalSection.h"
    typedef FMacCriticalSection FCriticalSection;
#else
    #include "Core/PlatformInterface/IPlatformCriticalSection.h"
    typedef IPlatformCriticalSection FCriticalSection;
#endif
