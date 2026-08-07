#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformSystemClipboard.h"
    typedef FWindowsPlatformSystemClipboard FPlatformSystemClipboard;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformSystemClipboard.h"
    typedef FMacPlatformSystemClipboard FPlatformSystemClipboard;
#else
    #include "Core/Generic/GenericPlatformSystemClipboard.h"
	typedef FGenericPlatformSystemClipboard FPlatformSystemClipboard;
#endif
