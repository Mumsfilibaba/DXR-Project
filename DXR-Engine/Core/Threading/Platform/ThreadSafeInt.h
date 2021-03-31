#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsThreadSafeInt.h"
#else
    #error No Platform Defined
#endif