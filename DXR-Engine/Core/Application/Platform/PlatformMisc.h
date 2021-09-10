#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Windows/WindowsMisc.h"
typedef WindowsMisc PlatformMisc;

#elif defined(PLATFORM_MACOS)
#include "Core/Application/Generic/GenericMisc.h"
typedef GenericMisc PlatformMisc;
// TODO: Create MacMisc

#else
#include "Core/Application/Generic/GenericMisc.h"
typedef GenericMisc PlatformMisc;

#endif
