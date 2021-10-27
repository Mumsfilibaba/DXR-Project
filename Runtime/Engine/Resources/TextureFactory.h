#pragma once
#include "RHI/RHITypes.h"

#include "Engine/EngineAPI.h"

#include "Core/Utilities/StringUtilities.h"

enum ETextureFactoryFlags : uint32
{
    TextureFactoryFlag_None = 0,
    TextureFactoryFlag_GenerateMips = FLAG( 1 ),
};

class ENGINE_API CTextureFactory
{
public:
    static bool Init();
    static void Release();

    // TODO: Supports R8G8B8A8 and R32G32B32A32 for now, support more formats? Such as Float16?
    static class CRHITexture2D* LoadFromImage2D( struct SImage2D* InImage, uint32 CreateFlags );
    static class CRHITexture2D* LoadFromFile( const CString& Filepath, uint32 CreateFlags, EFormat Format );
    static class CRHITexture2D* LoadFromMemory( const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format );

    static class CRHITextureCube* CreateTextureCubeFromPanorma( class CRHITexture2D* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format );
};