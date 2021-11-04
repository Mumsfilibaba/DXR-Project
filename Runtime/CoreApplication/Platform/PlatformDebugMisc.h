#pragma once

#if defined(PLATFORM_WINDOWS)
#include "CoreApplication/Windows/WindowsDebugMisc.h"
typedef CWindowsDebugMisc PlatformDebugMisc;

#elif defined(PLATFORM_MACOS)
#include "CoreApplication/Mac/MacDebugMisc.h"
typedef CMacDebugMisc PlatformDebugMisc;

#else
#include "CoreApplication/Platform/PlatformDebugMisc.h"
typedef CPlatformDebugMisc PlatformDebugMisc;

#endif