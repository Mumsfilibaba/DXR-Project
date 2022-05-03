#pragma once
#include "Core/Core.h"

#if MONOLITHIC_BUILD
    #define RHI_API
#else
    #if RHI_IMPL
        #define RHI_API MODULE_EXPORT
    #else
        #define RHI_API MODULE_IMPORT
    #endif
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Config

// TODO: Should be in a config file
#define ENABLE_API_DEBUGGING       (0)
#define ENABLE_API_GPU_DEBUGGING   (0) // D3D12
#define ENABLE_API_GPU_BREADCRUMBS (0) // D3D12

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Constants

enum : uint32
{
    // Maximums
    kRHIMaxRenderTargetCount    = 8,
    kRHIMaxLocalShaderBindings  = 4,
    kRHIMaxShaderConstants      = 32,
    kRHIMaxVertexBuffers        = 32,

    // Other
    kRHIAllRemainingMipLevels   = uint32(~0),
    kRHIAllRemainingArraySlices = uint32(~0),

    kRHINumCubeFaces            = 6,
};