#pragma once
#include "RenderingCore/RenderingCore.h"
#include "RenderingCore/Format.h"

#include "Containers/String.h"

/*
* ETextureFactoryFlags
*/

enum ETextureFactoryFlags : Uint32
{
	TEXTURE_FACTORY_FLAGS_NONE			= 0,
	TEXTURE_FACTORY_FLAGS_GENERATE_MIPS = FLAG(1),
};

class Texture2D;
class TextureCube;

/*
* TextureFactory
*/

class TextureFactory
{
public:
	// Supports R8G8B8A8 and R32G32B32A32 for now
	static Texture2D* LoadFromFile(const std::string& Filepath, Uint32 CreateFlags, EFormat Format);
	static Texture2D* LoadFromMemory(const Byte* Pixels, Uint32 Width, Uint32 Height, Uint32 CreateFlags, EFormat Format);

	static TextureCube* CreateTextureCubeFromPanorma(Texture2D* PanoramaSource, Uint32 CubeMapSize, Uint32 CreateFlags, EFormat Format);
};