#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsCriticalSection.h"
    typedef FWindowsCriticalSection FCriticalSection;
#elif PLATFORM_MACOS
    #include "Core/Threading/Mac/MacCriticalSection.h"
    typedef FMacCriticalSection FCriticalSection;
#else
    #include "Core/Threading/Generic/GenericCriticalSection.h"
    typedef FGenericCriticalSection FCriticalSection;
#endif
