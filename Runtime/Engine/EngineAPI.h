#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if ENGINE_API_EXPORT
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)

#endif

#else
#define ENGINE_API
#endif