#pragma once
#include "Defines.h"
#include "Types.h"

#include "STL/String.h"

class D3D12Device;
class D3D12Texture;

enum ETextureFactoryFlags : Uint32
{
	TEXTURE_FACTORY_FLAGS_NONE			= 0,
	TEXTURE_FACTORY_FLAGS_GENERATE_MIPS = FLAG(1),
};

class TextureFactory
{
public:
	// Supports R8G8B8A8 and R32G32B32A32 for now
	static D3D12Texture* LoadFromFile(D3D12Device* Device, const std::string& Filepath, Uint32 CreateFlags, DXGI_FORMAT Format);
	static D3D12Texture* LoadFromMemory(D3D12Device* Device, const Byte* Pixels, Uint32 Width, Uint32 Height, Uint32 CreateFlags, DXGI_FORMAT Format);

	static D3D12Texture* CreateTextureCubeFromPanorma(D3D12Device* Device, D3D12Texture* PanoramaSource, Uint32 CubeMapSize, DXGI_FORMAT Format);
};