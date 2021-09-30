#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsThread.h"
typedef CWindowsThread PlatformThread;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Mac/MacThread.h"
typedef CMacThread PlatformThread;

#else
#include "Core/Threading/Core/CoreThread.h"
typedef CCoreThread PlatformThread;

#endif
