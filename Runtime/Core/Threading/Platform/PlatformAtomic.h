#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Windows/WindowsAtomic.h"
typedef CWindowsAtomic PlatformAtomic;

#elif PLATFORM_MACOS
#include "Core/Threading/Mac/MacAtomic.h"
typedef CMacAtomic PlatformAtomic;

#else
#include "Core/Threading/Interface/PlatformAtomic.h"
typedef CPlatformAtomic PlatformAtomic;

#endif