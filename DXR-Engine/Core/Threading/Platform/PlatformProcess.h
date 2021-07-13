#pragma once
#ifdef PLATFORM_WINDOWS
#include "Core/Threading/Windows/WindowsProcess.h"
typedef WindowsProcess PlatformProcess;
#else
#include "Core/Threading/Generic/GenericProcess.h"
typedef GenericProcess PlatformProcess;
#endif