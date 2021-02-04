#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Windows/WindowsDebugMisc.h"
    typedef WindowsDebugMisc PlatformDebugMisc;
#else
    #include "Debug/GenericDebugMisc.h"
    typedef GenericDebugMisc PlatformDebugMisc;
#endif