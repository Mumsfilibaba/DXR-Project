#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if RHI_API_EXPORT
#define RHI_API __declspec(dllexport)
#else
#define RHI_API __declspec(dllimport)
#endif

#else 
#define RHI_API

#endif