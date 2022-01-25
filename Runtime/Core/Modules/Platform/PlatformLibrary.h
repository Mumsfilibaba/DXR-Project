#pragma once

#if PLATFORM_WINDOWS
#include "Core/Modules/Windows/WindowsLibrary.h"
typedef CWindowsLibrary PlatformLibrary;

#elif PLATFORM_MACOS
#include "Core/Modules/Mac/MacLibrary.h"
typedef CMacLibrary PlatformLibrary;

#else
#include "Core/Modules/Interface/PlatformLibraryInterface.h"
typedef CPlatformLibraryInterface PlatformLibrary;

#endif