#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsAtomic.h"
    typedef FWindowsAtomic FPlatformAtomic;
#elif PLATFORM_MACOS
    #include "Core/Threading/Mac/MacAtomic.h"
    typedef FMacAtomic FPlatformAtomic;
#else
    #include "Core/Threading/Generic/GenericAtomic.h"
    typedef FGenericAtomic FPlatformAtomic;
#endif