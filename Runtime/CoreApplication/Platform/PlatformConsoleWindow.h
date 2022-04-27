#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsConsoleWindow.h"
    typedef CWindowsConsoleWindow PlatformConsoleWindow;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacConsoleWindow.h"
    typedef CMacConsoleWindow PlatformConsoleWindow;
#else
    #include "CoreApplication/Generic/GenericConsoleWindow.h"
    typedef CGenericConsoleWindow PlatformConsoleWindow;
#endif