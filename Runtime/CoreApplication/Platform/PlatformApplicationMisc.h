#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Windows/WindowsApplicationMisc.h"
typedef CWindowsApplicationMisc PlatformApplicationMisc;

#elif defined(PLATFORM_MACOS)
#include "Core/Application/Mac/MacApplicationMisc.h"
typedef CMacApplicationMisc PlatformApplicationMisc;

#else
#include "Core/Application/Core/CoreApplicationMisc.h"
typedef CCoreApplicationMisc PlatformApplicationMisc;

#endif
