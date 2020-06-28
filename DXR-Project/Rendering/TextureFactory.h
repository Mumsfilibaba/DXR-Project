#pragma once
#include "Defines.h"
#include "Types.h"

#include "STL/String.h"

class D3D12Device;
class D3D12Texture;

class TextureFactory
{
public:
	static D3D12Texture* LoadFromFile(D3D12Device* Device, const std::string& Filepath);
	static D3D12Texture* LoadFromMemory(D3D12Device* Device, const Byte* Pixels, Uint32 Width, Uint32 Height);
};