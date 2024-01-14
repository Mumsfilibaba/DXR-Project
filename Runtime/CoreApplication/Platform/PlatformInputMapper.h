#pragma once

#if PLATFORM_WINDOWS
    #include "CoreApplication/Windows/WindowsInputMapper.h"
    typedef FWindowsInputMapper FPlatformInputMapper;
#elif PLATFORM_MACOS
    #include "CoreApplication/Mac/MacInputMapper.h"
    typedef FMacInputMapper FPlatformInputMapper;
#else
    #include "CoreApplication/Generic/GenericInputMapper.h"
    typedef EGenericInputMapper FPlatformInputMapper;
#endif
