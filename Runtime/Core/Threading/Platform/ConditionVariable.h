#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Windows/WindowsConditionVariable.h"
typedef CWindowsConditionVariable CConditionVariable;

#elif PLATFORM_MACOS
#include "Core/Threading/Mac/MacConditionVariable.h"
typedef CMacConditionVariable CConditionVariable;

#else
#include "Core/Threading/Interface/PlatformConditionVariable.h"
typedef CPlatformConditionVariable CConditionVariable;

#endif