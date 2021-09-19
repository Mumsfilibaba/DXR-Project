#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Windows/WindowsApplication.h"
typedef CWindowsApplication PlatformApplication;

#elif defined(PLATFORM_MACOS)
#include "Core/Application/Mac/MacApplication.h"
typedef CMacApplication PlatformApplication;

#else
#include "Core/Application/Generic/GenericApplication.h"
typedef CGenericApplication PlatformApplication;

#endif