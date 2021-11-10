#pragma once

#if PLATFORM_WINDOWS
#include "CoreApplication/Windows/WindowsApplicationMisc.h"
typedef CWindowsApplicationMisc PlatformApplicationMisc;

#elif PLATFORM_MACOS
#include "CoreApplication/Mac/MacApplicationMisc.h"
typedef CMacApplicationMisc PlatformApplicationMisc;

#else
#include "CoreApplication/Interface/PlatformApplicationMisc.h"
typedef CPlatformApplicationMisc PlatformApplicationMisc;

#endif
