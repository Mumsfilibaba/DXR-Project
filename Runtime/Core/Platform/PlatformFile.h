#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsPlatformFile.h"
    typedef FWindowsPlatformFile FPlatformFile;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacPlatformFile.h"
    typedef FMacPlatformFile FPlatformFile;
#else
    #include "Core/Generic/GenericFile.h"
    typedef FGenericPlatformFile FPlatformFile;
#endif