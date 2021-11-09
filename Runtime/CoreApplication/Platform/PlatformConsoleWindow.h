#pragma once

#if PLATFORM_WINDOWS
#include "CoreApplication/Windows/WindowsConsoleWindow.h"
typedef CWindowsConsoleWindow PlatformConsoleWindow;

#elif PLATFORM_MACOS
#include "CoreApplication/Mac/MacConsoleWindow.h"
typedef CMacConsoleWindow PlatformConsoleWindow;

#else
#include "CoreApplication/Interface/PlatformConsoleWindow.h"
typedef CPlatformConsoleWindow PlatformConsoleWindow;

#endif