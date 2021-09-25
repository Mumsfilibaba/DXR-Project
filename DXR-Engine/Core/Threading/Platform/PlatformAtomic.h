#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsAtomic.h"
typedef CWindowsAtomic PlatformAtomic;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Mac/MacAtomic.h"
typedef CMacAtomic PlatformAtomic;

#else
#include "Core/Threading/Generic/GenericAtomic.h"
typedef CGenericAtomic PlatformAtomic;

#endif