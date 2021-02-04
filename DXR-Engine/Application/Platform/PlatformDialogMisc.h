#pragma once
#ifdef PLATFORM_WINDOWS
    #include "Windows/WindowsDialogMisc.h"
    typedef WindowsDialogMisc PlatformDialogMisc;
#else
    #include "Application/Generic/GenericDialogMisc.h"
    typedef GenericDialogMisc PlatformDialogMisc;
#endif