#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if SANDBOX_EXPORT
#define SANDBOX_API __declspec(dllexport)
#else
#define SANDBOX_API __declspec(dllimport)
#endif

#else
#define SANDBOX_API
#endif