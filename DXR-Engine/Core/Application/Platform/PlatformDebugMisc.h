#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Windows/WindowsDebugMisc.h"
typedef CWindowsDebugMisc PlatformDebugMisc;

#elif defined(PLATFORM_MACOS)
#include "Core/Application/Mac/MacDebugMisc.h"
typedef CMacDebugMisc PlatformDebugMisc;

#else
#include "Core/Application/Generic/GenericDebugMisc.h"
typedef CGenericDebugMisc PlatformDebugMisc;

#endif