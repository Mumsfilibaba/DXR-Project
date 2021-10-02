#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Time/Windows/WindowsTime.h"
typedef CWindowsTime PlatformTime;

#elif defined(PLATFORM_MACOS)
#include "Core/Time/Mac/MacTime.h"
typedef CMacTime PlatformTime;

#else
#include "Core/Time/Generic/GenericTime.h"
typedef CCoreTime PlatformTime;

#endif
