#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsConsoleOutputDevice.h"
    typedef FWindowsConsoleOutputDevice FPlatformConsoleOutputDevice;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacConsoleOutputDevice.h"
    typedef FMacConsoleOutputDevice FPlatformConsoleOutputDevice;
#else
    #include "CoreApplication/Generic/GenericConsoleOutputDevice.h"
    typedef FGenericConsoleOutputDevice FPlatformConsoleOutputDevice;
#endif
