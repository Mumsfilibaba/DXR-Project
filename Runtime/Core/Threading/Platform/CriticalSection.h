#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsCriticalSection.h"
    typedef CWindowsCriticalSection CCriticalSection;
#elif PLATFORM_MACOS
    #include "Core/Threading/Mac/MacCriticalSection.h"
    typedef CMacCriticalSection CCriticalSection;
#else
    #include "Core/Threading/Generic/GenericCriticalSection.h"
    typedef CGenericCriticalSection CCriticalSection;
#endif
