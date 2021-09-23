#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsThreadMisc.h"
typedef CWindowsThreadMisc PlatformThreadMisc;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Mac/MacThreadMisc.h"
typedef CMacThreadMisc PlatformThreadMisc;

#else
#include "Core/Threading/Generic/GenericThreadMisc.h"
typedef CGenericThreadMisc PlatformThreadMisc;

#endif