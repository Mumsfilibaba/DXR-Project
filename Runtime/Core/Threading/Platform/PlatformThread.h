#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsThread.h"
typedef CWindowsThread PlatformThread;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Mac/MacThread.h"
typedef CMacThread PlatformThread;

#else
#include "Core/Threading/Interface/PlatformThread.h"
typedef CPlatformThread PlatformThread;

#endif
