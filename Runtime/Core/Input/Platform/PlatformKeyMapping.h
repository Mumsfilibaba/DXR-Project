#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Input/Windows/WindowsKeyMapping.h"
    typedef FWindowsKeyMapping FPlatformKeyMapping;
#elif PLATFORM_MACOS
    #include "Core/Input/Mac/MacKeyMapping.h"
    typedef FMacKeyMapping FPlatformKeyMapping;
#else
    #include "Core/Input/Generic/GenericKeyMapping.h"
    typedef FGenericKeyMapping FPlatformKeyMapping;
#endif
