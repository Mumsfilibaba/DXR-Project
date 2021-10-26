#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if D3D12_API_EXPORT
#define D3D12_API __declspec(dllexport)
#else
#define D3D12_API
#endif

#else
#define D3D12_API
#endif