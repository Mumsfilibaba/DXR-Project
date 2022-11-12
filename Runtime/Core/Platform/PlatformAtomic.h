#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformAtomic.h"
    typedef FWindowsPlatformAtomic FPlatformAtomic;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacAtomic.h"
    typedef FMacPlatformAtomic FPlatformAtomic;
#else
    #include "Core/Generic/GenericPlatformAtomic.h"
    typedef FGenericPlatformAtomic FPlatformAtomic;
#endif