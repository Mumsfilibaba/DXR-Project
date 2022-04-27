#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Input/Windows/WindowsKeyMapping.h"
    typedef CWindowsKeyMapping PlatformKeyMapping;
#elif PLATFORM_MACOS
    #include "Core/Input/Mac/MacKeyMapping.h"
    typedef CMacKeyMapping PlatformKeyMapping;
#else
    #include "Core/Input/Generic/GenericKeyMapping.h"
    typedef CGenericKeyMapping PlatformKeyMapping;
#endif
