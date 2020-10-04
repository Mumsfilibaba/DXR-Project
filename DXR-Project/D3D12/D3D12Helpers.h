#pragma once
#include "RenderingCore/Buffer.h"

#include <d3d12.h>

inline D3D12_RESOURCE_FLAGS ConvertBufferFlags(BufferFlags Flags)
{
	D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	if (!(Flags & EBufferFlag::BUFFER_FLAG_SHADER_RESOURCE))
	{
		Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	}
	
	if (Flags & EBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS)
	{
		Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return Result;
}