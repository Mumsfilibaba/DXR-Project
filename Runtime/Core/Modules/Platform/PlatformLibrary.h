#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Modules/Windows/WindowsLibrary.h"
typedef CWindowsLibrary PlatformLibrary;

#elif defined(PLATFORM_MACOS)
#include "Core/Modules/Mac/MacLibrary.h"
typedef CMacLibrary PlatformLibrary;

#else
#include "Core/Modules/Interface/PlatformLibrary.h"
typedef CPlatformLibrary PlatformLibrary;

#endif