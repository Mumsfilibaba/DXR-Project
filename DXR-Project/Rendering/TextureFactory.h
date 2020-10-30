#pragma once
#include "RenderingCore/RenderingCore.h"
#include "RenderingCore/Format.h"

#include "Containers/String.h"

/*
* ETextureFactoryFlags
*/

enum ETextureFactoryFlags : Uint32
{
	TextureFactoryFlag_None			= 0,
	TextureFactoryFlag_GenerateMips = FLAG(1),
};

class Texture2D;
class TextureCube;

/*
* TextureFactory
*/

class TextureFactory
{
public:
	static bool Initialize();
	static void Release();

	// Supports R8G8B8A8 and R32G32B32A32 for now
	static Texture2D* LoadFromFile(const std::string& Filepath, Uint32 CreateFlags, EFormat Format);
	static Texture2D* LoadFromMemory(const Byte* Pixels, Uint32 Width, Uint32 Height, Uint32 CreateFlags, EFormat Format);

	static TextureCube* CreateTextureCubeFromPanorma(Texture2D* PanoramaSource, Uint32 CubeMapSize, Uint32 CreateFlags, EFormat Format);
};