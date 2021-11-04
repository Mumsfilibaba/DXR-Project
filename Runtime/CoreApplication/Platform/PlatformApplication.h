#pragma once

#if defined(PLATFORM_WINDOWS)
#include "CoreApplication/Windows/WindowsApplication.h"
typedef CWindowsApplication PlatformApplication;

#elif defined(PLATFORM_MACOS)
#include "CoreApplication/Mac/MacApplication.h"
typedef CMacApplication PlatformApplication;

#else
#include "CoreApplication/Interface/PlatformApplication.h"
typedef CPlatformApplication PlatformApplication;

#endif