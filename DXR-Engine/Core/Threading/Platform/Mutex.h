#pragma once
#ifdef PLATFORM_WINDOWS
#include "Core/Threading/Windows/WindowsMutex.h"
typedef CWindowsMutex Mutex;
#else
#error No Platform Defined
#endif