#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsCriticalSection.h"
typedef CWindowsCriticalSection CCriticalSection;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Generic/GenericCriticalSection.h"
typedef CGenericCriticalSection CCriticalSection;
// TODO: MacMutex

#else
#include "Core/Threading/Generic/GenericCriticalSection.h"
typedef CGenericCriticalSection CCriticalSection;

#endif
