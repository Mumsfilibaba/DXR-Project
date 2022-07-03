#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Modules/Windows/WindowsLibrary.h"
    typedef FWindowsLibrary FPlatformLibrary;
#elif PLATFORM_MACOS
    #include "Core/Modules/Mac/MacLibrary.h"
    typedef FMacLibrary FPlatformLibrary;
#else
    #include "Core/Modules/Generic/GenericLibrary.h"
    typedef FGenericLibrary FPlatformLibrary;
#endif