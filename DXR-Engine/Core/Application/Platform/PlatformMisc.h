#pragma once
#ifdef PLATFORM_WINDOWS
#include "Core/Application/Windows/WindowsMisc.h"
typedef WindowsMisc PlatformMisc;
#else
#include "Core/Application/Generic/Misc.h"
typedef Misc PlatformMisc;
#endif