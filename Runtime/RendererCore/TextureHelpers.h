#pragma once
#include "Core/Math/Math.h"

struct TextureHelpers
{
    static FORCEINLINE uint32 TextureSizeToMiplevels(uint32 TextureSize)
    {
        return Math::Max<uint32>(static_cast<uint32>(Math::Log2(static_cast<float>(TextureSize))), 1u);
    }
};
