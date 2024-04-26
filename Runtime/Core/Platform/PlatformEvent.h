#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsEvent.h"
    typedef FWindowsEvent FPlatformEvent;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacEvent.h"
    typedef FMacEvent FPlatformEvent;
#else
    #include "Core/Generic/GenericEvent.h"
    typedef FGenericEvent FPlatformEvent;
#endif