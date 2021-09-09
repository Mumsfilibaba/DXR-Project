#pragma once
#ifdef PLATFORM_WINDOWS
#include "Core/Threading/Windows/WindowsConditionVariable.h"
typedef CWindowsConditionVariable ConditionVariable;
#else
#error No Platform Defined
#endif