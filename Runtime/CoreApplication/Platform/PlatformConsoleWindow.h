#pragma once

#if defined(PLATFORM_WINDOWS)
#include "CoreApplication/Windows/WindowsOutputConsole.h"
typedef CWindowsOutputConsole PlatformOutputConsole;

#elif defined(PLATFORM_MACOS)
#include "CoreApplication/Mac/MacOutputConsole.h"
typedef CMacOutputConsole PlatformOutputConsole;

#else
#include "CoreApplication/Interface/PlatformConsoleWindow.h"
typedef CPlatformConsoleWindow PlatformOutputConsole;

#endif