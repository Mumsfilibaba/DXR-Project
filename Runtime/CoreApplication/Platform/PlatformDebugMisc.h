#pragma once

#if PLATFORM_WINDOWS
#include "CoreApplication/Windows/WindowsDebugMisc.h"
typedef CWindowsDebugMisc PlatformDebugMisc;

#elif PLATFORM_MACOS
#include "CoreApplication/Mac/MacDebugMisc.h"
typedef CMacDebugMisc PlatformDebugMisc;

#else
#include "CoreApplication/Platform/PlatformDebugMisc.h"
typedef CPlatformDebugMisc PlatformDebugMisc;

#endif