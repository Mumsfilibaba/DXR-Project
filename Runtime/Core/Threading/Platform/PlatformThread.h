#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsThread.h"
    typedef FWindowsThread FPlatformThread;
#elif PLATFORM_MACOS
    #include "Core/Threading/Mac/MacThread.h"
    typedef FMacThread FPlatformThread;
#else
    #include "Core/Threading/Generic/GenericThread.h"
    typedef FGenericThread FPlatformThread;
#endif
