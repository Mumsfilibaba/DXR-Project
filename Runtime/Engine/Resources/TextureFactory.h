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

class ENGINE_API FTextureFactory
{
public:
    static bool Init();
    static void Release();

    // TODO: Supports R8G8B8A8 and R32G32B32A32 for now, support more formats? Such as Float16?
    static class FRHITexture2D* LoadFromImage2D(struct FImage2D* InImage, uint32 CreateFlags);
    static class FRHITexture2D* LoadFromFile(const FString& Filepath, uint32 CreateFlags, EFormat Format);
    static class FRHITexture2D* LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format);

    static class FRHITextureCube* CreateTextureCubeFromPanorma(class FRHITexture2D* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format);
};