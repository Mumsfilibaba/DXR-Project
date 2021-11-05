#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsCriticalSection.h"
typedef CWindowsCriticalSection CCriticalSection;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Mac/MacCriticalSection.h"
typedef CMacCriticalSection CCriticalSection;

#else
#include "Core/Threading/Interface/PlatformCriticalSection.h"
typedef CPlatformCriticalSection CCriticalSection;

#endif
