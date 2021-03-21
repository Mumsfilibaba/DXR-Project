#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Application/Windows/WindowsMisc.h"
    typedef WindowsMisc PlatformMisc;
#else
    #include "Core/Application/Generic/GenericMisc.h"
    typedef GenericMisc PlatformMisc;
#endif