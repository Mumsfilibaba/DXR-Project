#pragma once

#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include "Core/Windows/Windows.h"
    #include <dxgi1_6.h>
    #include <d3d12.h>
    #include <wrl/client.h>
#else
    #error Windows precompiled included on non-Windows platform
#endif