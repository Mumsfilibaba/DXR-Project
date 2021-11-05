#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if RENDERER_API_EXPORT
#define RENDERER_API __declspec(dllexport)
#else
#define RENDERER_API __declspec(dllimport)
#endif

#else
#define RENDERER_API
#endif