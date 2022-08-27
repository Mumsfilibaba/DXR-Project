#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsAtomic.h"
    typedef FWindowsAtomic FPlatformAtomic;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacAtomic.h"
    typedef FMacAtomic FPlatformAtomic;
#else
    #include "Core/Generic/GenericAtomic.h"
    typedef FGenericAtomic FPlatformAtomic;
#endif