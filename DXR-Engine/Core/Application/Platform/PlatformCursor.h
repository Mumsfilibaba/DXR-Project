#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Core/Application/Windows/WindowsCursor.h"
    typedef WindowsCursor PlatformCursor;
#else
    #include "Core/Application/Generic/GenericCursor.h"
    typedef GenericCursor PlatformCursor;
#endif