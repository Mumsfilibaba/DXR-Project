#pragma once

#if defined(PLATFORM_WINDOWS)

#define NOMINMAX
#include "Windows.h"
#include "Windows.inl"

#include <dxgi1_6.h>
#include <d3d12.h>

#include <wrl/client.h>

#else
#error Windows precompiled included on non-Windows platform
#endif