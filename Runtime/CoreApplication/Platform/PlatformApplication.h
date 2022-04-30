#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsApplication.h"
    typedef CWindowsApplication PlatformApplication;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacApplication.h"
    typedef CMacApplication PlatformApplication;
#else
    #include "CoreApplication/Generic/GenericApplication.h"
    typedef CGenericApplication PlatformApplication;
#endif
