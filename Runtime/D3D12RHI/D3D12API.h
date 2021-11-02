#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if D3D12RHI_API_EXPORT
#define D3D12RHI_API __declspec(dllexport)
#else
#define D3D12RHI_API
#endif

#else
#define D3D12RHI_API
#endif