#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsConsoleWindow.h"
    typedef FWindowsConsoleWindow FPlatformConsoleWindow;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacConsoleWindow.h"
    typedef FMacConsoleWindow FPlatformConsoleWindow;
#else
    #include "CoreApplication/PlatformInterface/IPlatformConsoleWindow.h"
    typedef IPlatformConsoleWindow FPlatformConsoleWindow;
#endif
