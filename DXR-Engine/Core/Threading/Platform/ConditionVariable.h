#pragma once
#ifdef PLATFORM_WINDOWS
#include "Core/Threading/Windows/WindowsConditionVariable.h"
#else
#error No Platform Defined
#endif