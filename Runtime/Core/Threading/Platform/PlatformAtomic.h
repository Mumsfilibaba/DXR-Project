#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsAtomic.h"
    typedef CWindowsAtomic PlatformAtomic;
#elif PLATFORM_MACOS
    #include "Core/Threading/Mac/MacAtomic.h"
    typedef CMacAtomic PlatformAtomic;
#else
    #include "Core/Threading/Generic/PlatformAtomic.h"
    typedef CGenericAtomic PlatformAtomic;
#endif