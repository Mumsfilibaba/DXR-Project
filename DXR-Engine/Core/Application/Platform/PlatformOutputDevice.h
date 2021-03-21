#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Application/Windows/WindowsConsoleOutput.h"
    typedef WindowsConsoleOutput PlatformOutputDevice;
#else
    #include "Core/Application/Generic/GenericOutputDevice.h"
    typedef GenericOutputDevice PlatformOutputDevice;
#endif