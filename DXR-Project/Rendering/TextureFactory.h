#pragma once
#include "RenderingCore/RenderingCore.h"
#include "RenderingCore/Format.h"

#include "Containers/String.h"

/*
* ETextureFactoryFlags
*/

enum ETextureFactoryFlags : UInt32
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
	static Texture2D* LoadFromFile(const std::string& Filepath, UInt32 CreateFlags, DXGI_FORMAT Format);
	static Texture2D* LoadFromMemory(const Byte* Pixels, UInt32 Width, UInt32 Height, UInt32 CreateFlags, DXGI_FORMAT Format);

	static TextureCube* CreateTextureCubeFromPanorma(D3D12Texture* PanoramaSource, UInt32 CubeMapSize, UInt32 CreateFlags, DXGI_FORMAT Format);
};