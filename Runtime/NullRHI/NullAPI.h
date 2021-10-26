#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if NULL_API_EXPORT
#define NULL_API __declspec(dllexport)
#else
#define NULL_API 
#endif

#else
#define NULL_API 
#endif