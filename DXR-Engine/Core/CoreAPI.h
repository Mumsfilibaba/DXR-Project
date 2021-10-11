#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if CORE_API_EXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API
#endif

#else
#define CORE_API
#endif