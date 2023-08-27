#pragma once
#include "Core/Core.h"

enum : uint32
{
    // Other
    kRHIAllRemainingMipLevels   = uint32(~0),
    kRHIAllRemainingArraySlices = uint32(~0),

    kRHINumCubeFaces            = 6,
};

struct FRHILimits
{
    inline static constexpr uint32 MaxRenderTargetCount = 8;
    
    inline static constexpr uint32 MaxLocalShaderBindings = 4;
    
    inline static constexpr uint32 MaxShaderConstants = 32;

    inline static constexpr uint32 MaxVertexBuffers = 32;
};