#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if INTERFACE_API_EXPORT
#define INTERFACE_API __declspec(dllexport)
#else
#define INTERFACE_API __declspec(dllimport)

#endif

#else
#define INTERFACE_API
#endif