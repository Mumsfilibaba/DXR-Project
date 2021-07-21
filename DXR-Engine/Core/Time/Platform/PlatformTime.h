#pragma once
#ifdef PLATFORM_WINDOWS
#include "Time/Windows/WindowsTime.h"
typedef WindowsTime PlatformTime;
#else
#include "Time/Generic/GenericTime.h"
typedef GenericTime PlatformTime;
#endif