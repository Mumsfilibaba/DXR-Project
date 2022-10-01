#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Windows/WindowsFile.h"
    typedef FWindowsFile FPlatformFile;
#elif PLATFORM_MACOS
    #include "Core/Mac/MacFile.h"
    typedef FMacFile FPlatformFile;
#else
    #include "Core/Generic/GenericFile.h"
    typedef FGenericFile FPlatformFile;
#endif