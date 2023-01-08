#pragma once
#include "Core/Utilities/StringUtilities.h"
#include "RHI/RHITypes.h"
#include "Engine/Engine.h"

class FRHITexture;

enum ETextureFactoryFlags : uint32
{
    TextureFactoryFlag_None         = 0,
    TextureFactoryFlag_GenerateMips = FLAG(1),
};


struct ENGINE_API FTextureFactory
{
    static bool Init();
    static void Release();

    static FRHITexture* LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format);
    static FRHITexture* CreateTextureCubeFromPanorma(FRHITexture* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format);
};