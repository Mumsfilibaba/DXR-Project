#pragma once
#include "Defines.h"
#include "Types.h"

#include "Containers/String.h"

class D3D12Device;
class D3D12Texture;

/*
* ETextureFactoryFlags
*/

enum ETextureFactoryFlags : UInt32
{
	TEXTURE_FACTORY_FLAGS_NONE			= 0,
	TEXTURE_FACTORY_FLAGS_GENERATE_MIPS = FLAG(1),
};

/*
* TextureFactory
*/

class TextureFactory
{
public:
	// Supports R8G8B8A8 and R32G32B32A32 for now
	static D3D12Texture* LoadFromFile(const std::string& Filepath, UInt32 CreateFlags, DXGI_FORMAT Format);
	static D3D12Texture* LoadFromMemory(const Byte* Pixels, UInt32 Width, UInt32 Height, UInt32 CreateFlags, DXGI_FORMAT Format);

	static D3D12Texture* CreateTextureCubeFromPanorma(D3D12Texture* PanoramaSource, UInt32 CubeMapSize, UInt32 CreateFlags, DXGI_FORMAT Format);
};