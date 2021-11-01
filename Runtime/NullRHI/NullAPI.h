#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if NULLRHI_API_EXPORT
#define NULLRHI_API __declspec(dllexport)
#else
#define NULLRHI_API 
#endif

#else
#define NULLRHI_API 
#endif