#pragma once
#include "RenderingCore/Buffer.h"

#include <d3d12.h>

/*
* Converts EBufferUsage- flags to D3D12_RESOURCE_FLAGS
*/

inline D3D12_RESOURCE_FLAGS ConvertBufferUsage(Uint32 Usage)
{
	D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
	if (Usage & EBufferUsage::BufferUsage_UAV)
	{
		Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return Result;
}

/*
* Converts ETextureUsage- flags to D3D12_RESOURCE_FLAGS
*/

inline D3D12_RESOURCE_FLAGS ConvertTextureUsage(Uint32 Usage)
{
	D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
	if (Usage & ETextureUsage::TextureUsage_UAV)
	{
		Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	if (Usage & ETextureUsage::TextureUsage_RTV)
	{
		Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if (Usage & ETextureUsage::TextureUsage_DSV)
	{
		Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if (!(Usage & ETextureUsage::TextureUsage_SRV))
		{
			Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
	}

	return Result;
}

/*
* Converts EFormat to DXGI_FORMAT
*/

inline DXGI_FORMAT ConvertFormat(EFormat Format)
{
	switch (Format)
	{
	case EFormat::Format_R32G32B32A32_Typeless:		return DXGI_FORMAT_R32G32B32A32_TYPELESS;
	case EFormat::Format_R32G32B32A32_Float:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case EFormat::Format_R32G32B32A32_Uint:			return DXGI_FORMAT_R32G32B32A32_UINT;
	case EFormat::Format_R32G32B32A32_Sint:			return DXGI_FORMAT_R32G32B32A32_SINT;
	case EFormat::Format_R32G32B32_Typeless:		return DXGI_FORMAT_R32G32B32_TYPELESS;
	case EFormat::Format_R32G32B32_Float:			return DXGI_FORMAT_R32G32B32_FLOAT;
	case EFormat::Format_R32G32B32_Uint:			return DXGI_FORMAT_R32G32B32_UINT;
	case EFormat::Format_R32G32B32_Sint:			return DXGI_FORMAT_R32G32B32_SINT;
	case EFormat::Format_R16G16B16A16_Typeless:		return DXGI_FORMAT_R16G16B16A16_TYPELESS;
	case EFormat::Format_R16G16B16A16_Float:		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case EFormat::Format_R16G16B16A16_Unorm:		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case EFormat::Format_R16G16B16A16_Uint:			return DXGI_FORMAT_R16G16B16A16_UINT;
	case EFormat::Format_R16G16B16A16_Snorm:		return DXGI_FORMAT_R16G16B16A16_SNORM;
	case EFormat::Format_R16G16B16A16_Sint:			return DXGI_FORMAT_R16G16B16A16_SINT;
	case EFormat::Format_R32G32_Typeless:			return DXGI_FORMAT_R32G32_TYPELESS;
	case EFormat::Format_R32G32_Float:				return DXGI_FORMAT_R32G32_FLOAT;
	case EFormat::Format_R32G32_Uint:				return DXGI_FORMAT_R32G32_UINT;
	case EFormat::Format_R32G32_Sint:				return DXGI_FORMAT_R32G32_SINT;
	case EFormat::Format_R32G8X24_Typeless:			return DXGI_FORMAT_R32G8X24_TYPELESS;
	case EFormat::Format_D32_Float_S8X24_Uint:		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	case EFormat::Format_R32_Float_X8X24_Typeless:	return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	case EFormat::Format_X32_Typeless_G8X24_Uint:	return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
	case EFormat::Format_R10G10B10A2_Typeless:		return DXGI_FORMAT_R10G10B10A2_TYPELESS;
	case EFormat::Format_R10G10B10A2_Unorm:			return DXGI_FORMAT_R10G10B10A2_UNORM;
	case EFormat::Format_R10G10B10A2_Uint:			return DXGI_FORMAT_R10G10B10A2_UINT;
	case EFormat::Format_R11G11B10_Float:			return DXGI_FORMAT_R11G11B10_FLOAT;
	case EFormat::Format_R8G8B8A8_Typeless:			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case EFormat::Format_R8G8B8A8_Unorm:			return DXGI_FORMAT_R8G8B8A8_UNORM;
	case EFormat::Format_R8G8B8A8_Unorm_SRGB:		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case EFormat::Format_R8G8B8A8_Uint:				return DXGI_FORMAT_R8G8B8A8_UINT;
	case EFormat::Format_R8G8B8A8_Snorm:			return DXGI_FORMAT_R8G8B8A8_SNORM;
	case EFormat::Format_R8G8B8A8_Sint:				return DXGI_FORMAT_R8G8B8A8_SINT;
	case EFormat::Format_R16G16_Typeless:			return DXGI_FORMAT_R16G16_TYPELESS;
	case EFormat::Format_R16G16_Float:				return DXGI_FORMAT_R16G16_FLOAT;
	case EFormat::Format_R16G16_Unorm:				return DXGI_FORMAT_R16G16_UNORM;
	case EFormat::Format_R16G16_Uint:				return DXGI_FORMAT_R16G16_UINT;
	case EFormat::Format_R16G16_Snorm:				return DXGI_FORMAT_R16G16_SNORM;
	case EFormat::Format_R16G16_Sint:				return DXGI_FORMAT_R16G16_SINT;
	case EFormat::Format_R32_Typeless:				return DXGI_FORMAT_R32_TYPELESS;
	case EFormat::Format_D32_Float:					return DXGI_FORMAT_D32_FLOAT;
	case EFormat::Format_R32_Float:					return DXGI_FORMAT_R32_FLOAT;
	case EFormat::Format_R32_Uint:					return DXGI_FORMAT_R32_UINT;
	case EFormat::Format_R32_Sint:					return DXGI_FORMAT_R32_SINT;
	case EFormat::Format_R24G8_Typeless:			return DXGI_FORMAT_R24G8_TYPELESS;
	case EFormat::Format_D24_Unorm_S8_Uint:			return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case EFormat::Format_R24_Unorm_X8_Typeless:		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case EFormat::Format_X24_Typeless_G8_Uint:		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	case EFormat::Format_R8G8_Typeless:				return DXGI_FORMAT_R8G8_TYPELESS;
	case EFormat::Format_R8G8_Unorm:				return DXGI_FORMAT_R8G8_UNORM;
	case EFormat::Format_R8G8_Uint:					return DXGI_FORMAT_R8G8_UINT;
	case EFormat::Format_R8G8_Snorm:				return DXGI_FORMAT_R8G8_SNORM;
	case EFormat::Format_R8G8_Sint:					return DXGI_FORMAT_R8G8_SINT;
	case EFormat::Format_R16_Typeless:				return DXGI_FORMAT_R16_TYPELESS;
	case EFormat::Format_R16_Float:					return DXGI_FORMAT_R16_FLOAT;
	case EFormat::Format_D16_Unorm:					return DXGI_FORMAT_D16_UNORM;
	case EFormat::Format_R16_Unorm:					return DXGI_FORMAT_R16_UNORM;
	case EFormat::Format_R16_Uint:					return DXGI_FORMAT_R16_UINT;
	case EFormat::Format_R16_Snorm:					return DXGI_FORMAT_R16_SNORM;
	case EFormat::Format_R16_Sint:					return DXGI_FORMAT_R16_SINT;
	case EFormat::Format_R8_Typeless:				return DXGI_FORMAT_R8_TYPELESS;
	case EFormat::Format_R8_Unorm:					return DXGI_FORMAT_R8_UNORM;
	case EFormat::Format_R8_Uint:					return DXGI_FORMAT_R8_UINT;
	case EFormat::Format_R8_Snorm:					return DXGI_FORMAT_R8_SNORM;
	case EFormat::Format_R8_Sint:					return DXGI_FORMAT_R8_SINT;
	case EFormat::Format_A8_Unorm:					return DXGI_FORMAT_A8_UNORM;
	case EFormat::Format_R1_Unorm:					return DXGI_FORMAT_R1_UNORM;
	case EFormat::Format_B5G6R5_Unorm:				return DXGI_FORMAT_B5G6R5_UNORM;
	case EFormat::Format_B5G5R5A1_Unorm:			return DXGI_FORMAT_B5G5R5A1_UNORM;
	case EFormat::Format_B8G8R8A8_Unorm:			return DXGI_FORMAT_B8G8R8A8_UNORM;
	case EFormat::Format_B8G8R8X8_Unorm:			return DXGI_FORMAT_B8G8R8X8_UNORM;
	case EFormat::Format_B8G8R8A8_Typeless:			return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	case EFormat::Format_B8G8R8A8_Unorm_SRGB:		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	case EFormat::Format_B8G8R8X8_Typeless:			return DXGI_FORMAT_B8G8R8X8_TYPELESS;
	case EFormat::Format_B8G8R8X8_Unorm_SRGB:		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
	default: return DXGI_FORMAT_UNKNOWN;
	}
}