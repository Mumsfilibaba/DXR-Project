#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Windows/WindowsOutputConsole.h"
typedef CWindowsOutputConsole PlatformOutputConsole;

#elif defined(PLATFORM_MACOS)
#include "Core/Application/Mac/MacOutputConsole.h"
typedef CMacOutputConsole PlatformOutputConsole;

#else
#include "Core/Application/Generic/GenericOutputConsole.h"
typedef CGenericOutputConsole PlatformOutputConsole;

#endif