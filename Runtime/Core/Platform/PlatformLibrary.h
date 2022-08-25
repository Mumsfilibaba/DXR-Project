#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsLibrary.h"
    typedef FWindowsLibrary FPlatformLibrary;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacLibrary.h"
    typedef FMacLibrary FPlatformLibrary;
#else
    #include "Core/Generic/GenericLibrary.h"
    typedef FGenericLibrary FPlatformLibrary;
#endif