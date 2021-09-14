#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Windows/WindowsMisc.h"
typedef CWindowsMisc PlatformMisc;

#elif defined(PLATFORM_MACOS)
#include "Core/Application/Mac/MacMisc.h"
typedef CMacMisc PlatformMisc;

#else
#include "Core/Application/Generic/GenericMisc.h"
typedef CGenericMisc PlatformMisc;

#endif
