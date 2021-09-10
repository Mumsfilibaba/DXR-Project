#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Time/Windows/WindowsTime.h"
typedef WindowsTime PlatformTime;

#elif defined(PLATFORM_MACOS)
#include "Core/Time/Generic/GenericTime.h"
typedef GenericTime PlatformTime;
// TODO: MacTime

#else
#include "Core/Time/Generic/GenericTime.h"
typedef GenericTime PlatformTime;

#endif
