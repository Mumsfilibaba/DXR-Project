#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Application/Windows/WindowsApplication.h"
    typedef WindowsApplication PlatformApplication;
#else
    #include "Core/Application/Generic/GenericApplication.h"
    typedef GenericCursor PlatformCursor;
#endif