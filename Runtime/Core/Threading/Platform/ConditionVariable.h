#pragma once

#if PLATFORM_WINDOWS
    #include "Core/Threading/Windows/WindowsConditionVariable.h"
    typedef FWindowsConditionVariable FConditionVariable;
#elif PLATFORM_MACOS
    #include "Core/Threading/Mac/MacConditionVariable.h"
    typedef FMacConditionVariable FConditionVariable;
#else
    #include "Core/Threading/Generic/GenericConditionVariable.h"
    typedef FGenericConditionVariable FConditionVariable;
#endif
