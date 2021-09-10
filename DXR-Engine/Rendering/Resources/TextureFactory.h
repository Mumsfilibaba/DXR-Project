#pragma once
#include "RenderLayer/RenderingCore.h"

#include "Core/Utilities/StringUtilities.h"

enum ETextureFactoryFlags : uint32
{
    TextureFactoryFlag_None = 0,
    TextureFactoryFlag_GenerateMips = FLAG( 1 ),
};

class TextureFactory
{
public:
    static bool Init();
    static void Release();

    // TODO: Supports R8G8B8A8 and R32G32B32A32 for now, support more formats? Such as Float16?
    static class Texture2D* LoadFromFile( const std::string& Filepath, uint32 CreateFlags, EFormat Format );
    static class Texture2D* LoadFromMemory( const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format );

    static class TextureCube* CreateTextureCubeFromPanorma( class Texture2D* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format );
};