#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsKeyMapping.h"
    typedef FWindowsKeyMapping FPlatformKeyMapping;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacKeyMapping.h"
    typedef FMacKeyMapping FPlatformKeyMapping;
#else
    #include "CoreApplication/Generic/GenericKeyMapping.h"
    typedef FGenericKeyMapping FPlatformKeyMapping;
#endif
