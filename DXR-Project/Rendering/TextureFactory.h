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
};