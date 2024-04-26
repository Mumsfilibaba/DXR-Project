#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsThread.h"
    typedef FWindowsThread FPlatformThread;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacThread.h"
    typedef FMacThread FPlatformThread;
#else
    #include "Core/Generic/GenericThread.h"
    typedef FGenericThread FPlatformThread;
#endif