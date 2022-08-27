#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsOutputDeviceConsole.h"
    typedef FWindowsOutputDeviceConsole FPlatformOutputDeviceConsole;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacOutputDeviceConsole.h"
    typedef FMacOutputDeviceConsole FPlatformOutputDeviceConsole;
#else
    #include "CoreApplication/Misc/FOutputDeviceConsole.h"
    typedef FOutputDeviceConsole FPlatformOutputDeviceConsole;
#endif
