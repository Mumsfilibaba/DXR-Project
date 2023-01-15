#pragma once
#include "Core/Core.h"

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