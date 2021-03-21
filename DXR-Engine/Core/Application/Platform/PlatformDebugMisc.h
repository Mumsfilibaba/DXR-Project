#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Application/Windows/WindowsDebugMisc.h"
    typedef WindowsDebugMisc PlatformDebugMisc;
#else
    #include "Core/Application/Debug/GenericDebugMisc.h"
    typedef GenericDebugMisc PlatformDebugMisc;
#endif