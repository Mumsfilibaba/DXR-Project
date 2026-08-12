#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsConditionVariable.h"
    typedef FWindowsConditionVariable FConditionVariable;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacConditionVariable.h"
    typedef FMacConditionVariable FConditionVariable;
#else
    #include "Core/PlatformInterface/IPlatformConditionVariable.h"
    typedef IPlatformConditionVariable FConditionVariable;
#endif
