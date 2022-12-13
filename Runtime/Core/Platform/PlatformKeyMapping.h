#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsKeyMapping.h"
    typedef FWindowsKeyMapping FPlatformKeyMapping;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacKeyMapping.h"
    typedef FMacKeyMapping FPlatformKeyMapping;
#else
    #include "Core/Generic/GenericKeyMapping.h"
    typedef FGenericKeyMapping FPlatformKeyMapping;
#endif
