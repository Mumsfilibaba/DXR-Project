#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Windows/WindowsApplication.h"
    typedef WindowsApplication PlatformApplication;
#else
    #include "Application/Generic/GenericApplication.h"
    typedef GenericCursor PlatformCursor;
#endif