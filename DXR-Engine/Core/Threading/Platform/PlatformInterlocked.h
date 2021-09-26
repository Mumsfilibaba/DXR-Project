#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsInterlocked.h"
typedef CWindowsInterlocked PlatformInterlocked;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Mac/MacInterlocked.h"
typedef CMacInterlocked PlatformInterlocked;

#else
#include "Core/Threading/Generic/GenericInterlocked.h"
typedef CGenericInterlocked PlatformInterlocked;

#endif
