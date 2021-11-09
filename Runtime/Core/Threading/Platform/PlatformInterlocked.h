#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Windows/WindowsInterlocked.h"
typedef CWindowsInterlocked PlatformInterlocked;

#elif PLATFORM_MACOS
#include "Core/Threading/Mac/MacInterlocked.h"
typedef CMacInterlocked PlatformInterlocked;

#else
#include "Core/Threading/Interface/PlatformInterlocked.h"
typedef CPlatformInterlocked PlatformInterlocked;

#endif
