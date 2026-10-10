#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformSocket.h"
    typedef FWindowsPlatformSocket FPlatformSocket;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformSocket.h"
    typedef FMacPlatformSocket FPlatformSocket;
#else
    #include "Core/PlatformInterface/IPlatformSocket.h"
    typedef IPlatformSocket FPlatformSocket;
#endif
