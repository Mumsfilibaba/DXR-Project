#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsAtomic.h"
    typedef WindowsAtomic PlatformAtomic;
#else
    #include "Core/Threading/Generic/GenericAtomic.h"
    typedef GenericAtomic PlatformAtomic;
#endif