#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsMutex.h"
#else
    #error No Platform Defined
#endif