#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsOutputDeviceConsole.h"
    typedef FWindowsOutputDeviceConsole FPlatformConsoleWindow;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacConsoleWindow.h"
    typedef FMacOutputDeviceConsole FPlatformConsoleWindow;
#else
    #include "CoreApplication/Generic/GenericConsoleWindow.h"
    typedef FOutputDeviceConsole FPlatformConsoleWindow;
#endif