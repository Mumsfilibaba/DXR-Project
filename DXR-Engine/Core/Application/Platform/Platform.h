#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Application/Windows/WindowsPlatform.h"
    typedef WindowsPlatform Platform;
#else
    #include "Core/Application/Generic/GenericPlatform.h"
    typedef GenericPlatform Platform;
#endif