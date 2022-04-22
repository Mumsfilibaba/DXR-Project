#pragma once
#include "RHI/RHITypes.h"

#include "Engine/Engine.h"

#include "Core/Utilities/StringUtilities.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// ETextureFactoryFlags

enum class ETextureFactoryFlags : uint32
{
    None         = 0,
    GenerateMips = FLAG(1),
};

ENUM_CLASS_OPERATORS(ETextureFactoryFlags);

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CTextureFactory

class ENGINE_API CTextureFactory
{
public:
    static bool Init();
    static void Release();

    // TODO: Supports R8G8B8A8 and R32G32B32A32 for now, support more formats? Such as Float16?
    static class CRHITexture2D* LoadFromImage2D(struct SImage2D* InImage, ETextureFactoryFlags CreateFlags);
    static class CRHITexture2D* LoadFromFile(const String& Filepath, ETextureFactoryFlags CreateFlags, ERHIFormat Format);
    static class CRHITexture2D* LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, ETextureFactoryFlags CreateFlags, ERHIFormat Format);

    static class CRHITextureCube* CreateTextureCubeFromPanorma(class CRHITexture2D* PanoramaSource, uint32 CubeMapSize, ETextureFactoryFlags CreateFlags, ERHIFormat Format);
};