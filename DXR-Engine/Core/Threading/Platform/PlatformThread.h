#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsThread.h"
typedef CWindowsThread PlatformThread;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Generic/GenericThread.h"
typedef CGenericThread PlatformThread;

#else
#include "Core/Threading/Generic/GenericThread.h"
typedef CGenericThread PlatformThread;

#endif