#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if COREAPPLICATION_API_EXPORT
#define COREAPPLICATION_API __declspec(dllexport)
#else
#define COREAPPLICATION_API __declspec(dllimport)
#endif

#else
#define COREAPPLICATION_API
#endif