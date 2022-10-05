#pragma once
#include "RHI/RHITypes.h"

#include "Engine/Engine.h"

#include "Core/Utilities/StringUtilities.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// ETextureFactoryFlags

enum ETextureFactoryFlags : uint32
{
    TextureFactoryFlag_None = 0,
    TextureFactoryFlag_GenerateMips = FLAG(1),
};

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FTextureFactory

struct ENGINE_API FTextureFactory
{
    static bool Init();
    static void Release();

    static class FRHITexture2D* LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format);

    static class FRHITextureCube* CreateTextureCubeFromPanorma(class FRHITexture2D* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format);
};