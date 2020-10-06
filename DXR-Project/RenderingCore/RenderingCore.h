#pragma once
#include "Defines.h"
#include "Types.h"

/*
* EComparisonFunc
*/

enum class EComparisonFunc
{
	COMPARISON_FUNC_NEVER			= 1,
	COMPARISON_FUNC_LESS			= 2,
	COMPARISON_FUNC_EQUAL			= 3,
	COMPARISON_FUNC_LESS_EQUAL		= 4,
	COMPARISON_FUNC_GREATER			= 5,
	COMPARISON_FUNC_NOT_EQUAL		= 6,
	COMPARISON_FUNC_GREATER_EQUAL	= 7,
	COMPARISON_FUNC_ALWAYS			= 8
};

inline const Char* ToString(EComparisonFunc ComparisonFunc)
{
	switch (ComparisonFunc)
	{
	case EComparisonFunc::COMPARISON_FUNC_NEVER:			return "BLEND_FACTOR_ZERO";
	case EComparisonFunc::COMPARISON_FUNC_LESS:				return "BLEND_FACTOR_ONE";
	case EComparisonFunc::COMPARISON_FUNC_EQUAL:			return "BLEND_FACTOR_SRC_COLOR";
	case EComparisonFunc::COMPARISON_FUNC_LESS_EQUAL:		return "BLEND_FACTOR_INV_SRC_COLOR";
	case EComparisonFunc::COMPARISON_FUNC_GREATER:			return "BLEND_FACTOR_SRC_ALPHA";
	case EComparisonFunc::COMPARISON_FUNC_NOT_EQUAL:		return "BLEND_FACTOR_INV_SRC_ALPHA";
	case EComparisonFunc::COMPARISON_FUNC_GREATER_EQUAL:	return "BLEND_FACTOR_DEST_ALPHA";
	case EComparisonFunc::COMPARISON_FUNC_ALWAYS:			return "BLEND_FACTOR_INV_DEST_ALPHA";
	default:												return "";
	}
}

/*
* EShaderStageFlag
*/

typedef Uint32 EShaderStageFlags;
enum EShaderStageFlag : EShaderStageFlags
{
	SHADER_STAGE_FLAG_NONE			= 0,
	// Graphics Pipeline
	SHADER_STAGE_FLAG_VERTEX		= FLAG(1),
	SHADER_STAGE_FLAG_HULL			= FLAG(2),
	SHADER_STAGE_FLAG_DOMAIN		= FLAG(3),
	SHADER_STAGE_FLAG_GEOMETRY		= FLAG(4),
	SHADER_STAGE_FLAG_PIXEL			= FLAG(5),
	// Compute
	SHADER_STAGE_FLAG_COMPUTE		= FLAG(6),
	// Mesh Shaders
	SHADER_STAGE_FLAG_MESH			= FLAG(7),
	SHADER_STAGE_FLAG_AMPLIFICATION	= FLAG(8),
	// Ray Tracing
	SHADER_STAGE_FLAG_RAY_GEN		= FLAG(9),
	SHADER_STAGE_FLAG_HIT_SHADER	= FLAG(10),
	SHADER_STAGE_FLAG_MISS_SHADER	= FLAG(11),
};

/*
* EShaderLanguage
*/

enum class EShaderLanguange
{
	SHADER_LANGUAGE_NONE = 0,
	SHADER_LANGUAGE_HLSL = 1,
};

inline const Char* ToString(EShaderLanguange ShaderLanguange)
{
	switch (ShaderLanguange)
	{
	case EShaderLanguange::SHADER_LANGUAGE_NONE:	return "SHADER_LANGUAGE_NONE";
	case EShaderLanguange::SHADER_LANGUAGE_HLSL:	return "SHADER_LANGUAGE_HLSL";
	default:										return "";
	}
}

/*
* EPrimitveTopologyType
*/

enum class EPrimitveTopologyType
{
	PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED	= 0,
	PRIMITIVE_TOPOLOGY_TYPE_POINT		= 1,
	PRIMITIVE_TOPOLOGY_TYPE_LINE		= 2,
	PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE	= 3,
	PRIMITIVE_TOPOLOGY_TYPE_PATCH		= 4
};

inline const Char* ToString(EPrimitveTopologyType PrimitveTopologyType)
{
	switch (PrimitveTopologyType)
	{
	case EPrimitveTopologyType::PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED:	return "PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED";
	case EPrimitveTopologyType::PRIMITIVE_TOPOLOGY_TYPE_POINT:		return "PRIMITIVE_TOPOLOGY_TYPE_POINT";
	case EPrimitveTopologyType::PRIMITIVE_TOPOLOGY_TYPE_LINE:		return "PRIMITIVE_TOPOLOGY_TYPE_LINE";
	case EPrimitveTopologyType::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:	return "PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE";
	case EPrimitveTopologyType::PRIMITIVE_TOPOLOGY_TYPE_PATCH:		return "PRIMITIVE_TOPOLOGY_TYPE_PATCH";
	default:														return "";
	}
}

/*
* EFormat
*/

enum class EFormat
{
	FORMAT_UNKNOWN					= 0,
	FORMAT_R32G32B32A32_TYPELESS	= 1,
	FORMAT_R32G32B32A32_FLOAT		= 2,
	FORMAT_R32G32B32A32_UINT		= 3,
	FORMAT_R32G32B32A32_SINT		= 4,
	FORMAT_R32G32B32_TYPELESS		= 5,
	FORMAT_R32G32B32_FLOAT			= 6,
	FORMAT_R32G32B32_UINT			= 7,
	FORMAT_R32G32B32_SINT			= 8,
	FORMAT_R16G16B16A16_TYPELESS	= 9,
	FORMAT_R16G16B16A16_FLOAT		= 10,
	FORMAT_R16G16B16A16_UNORM		= 11,
	FORMAT_R16G16B16A16_UINT		= 12,
	FORMAT_R16G16B16A16_SNORM		= 13,
	FORMAT_R16G16B16A16_SINT		= 14,
	FORMAT_R32G32_TYPELESS			= 15,
	FORMAT_R32G32_FLOAT				= 16,
	FORMAT_R32G32_UINT				= 17,
	FORMAT_R32G32_SINT				= 18,
	FORMAT_R32G8X24_TYPELESS		= 19,
	FORMAT_D32_FLOAT_S8X24_UINT		= 20,
	FORMAT_R32_FLOAT_X8X24_TYPELESS	= 21,
	FORMAT_X32_TYPELESS_G8X24_UINT	= 22,
	FORMAT_R10G10B10A2_TYPELESS		= 23,
	FORMAT_R10G10B10A2_UNORM		= 24,
	FORMAT_R10G10B10A2_UINT			= 25,
	FORMAT_R11G11B10_FLOAT			= 26,
	FORMAT_R8G8B8A8_TYPELESS		= 27,
	FORMAT_R8G8B8A8_UNORM			= 28,
	FORMAT_R8G8B8A8_UNORM_SRGB		= 29,
	FORMAT_R8G8B8A8_UINT			= 30,
	FORMAT_R8G8B8A8_SNORM			= 31,
	FORMAT_R8G8B8A8_SINT			= 32,
	FORMAT_R16G16_TYPELESS			= 33,
	FORMAT_R16G16_FLOAT				= 34,
	FORMAT_R16G16_UNORM				= 35,
	FORMAT_R16G16_UINT				= 36,
	FORMAT_R16G16_SNORM				= 37,
	FORMAT_R16G16_SINT				= 38,
	FORMAT_R32_TYPELESS				= 39,
	FORMAT_D32_FLOAT				= 40,
	FORMAT_R32_FLOAT				= 41,
	FORMAT_R32_UINT					= 42,
	FORMAT_R32_SINT					= 43,
	FORMAT_R24G8_TYPELESS			= 44,
	FORMAT_D24_UNORM_S8_UINT		= 45,
	FORMAT_R24_UNORM_X8_TYPELESS	= 46,
	FORMAT_X24_TYPELESS_G8_UINT		= 47,
	FORMAT_R8G8_TYPELESS			= 48,
	FORMAT_R8G8_UNORM				= 49,
	FORMAT_R8G8_UINT				= 50,
	FORMAT_R8G8_SNORM				= 51,
	FORMAT_R8G8_SINT				= 52,
	FORMAT_R16_TYPELESS				= 53,
	FORMAT_R16_FLOAT				= 54,
	FORMAT_D16_UNORM				= 55,
	FORMAT_R16_UNORM				= 56,
	FORMAT_R16_UINT					= 57,
	FORMAT_R16_SNORM				= 58,
	FORMAT_R16_SINT					= 59,
	FORMAT_R8_TYPELESS				= 60,
	FORMAT_R8_UNORM					= 61,
	FORMAT_R8_UINT					= 62,
	FORMAT_R8_SNORM					= 63,
	FORMAT_R8_SINT					= 64,
	FORMAT_A8_UNORM					= 65,
	FORMAT_R1_UNORM					= 66,
	FORMAT_B5G6R5_UNORM				= 85,
	FORMAT_B5G5R5A1_UNORM			= 86,
	FORMAT_B8G8R8A8_UNORM			= 87,
	FORMAT_B8G8R8X8_UNORM			= 88,
	FORMAT_B8G8R8A8_TYPELESS		= 90,
	FORMAT_B8G8R8A8_UNORM_SRGB		= 91,
	FORMAT_B8G8R8X8_TYPELESS		= 92,
	FORMAT_B8G8R8X8_UNORM_SRGB		= 93,
};

inline const Char* ToString(EFormat Format)
{
	switch (Format)
	{
	case EFormat::FORMAT_R32G32B32A32_TYPELESS:		return "FORMAT_R32G32B32A32_TYPELESS";
	case EFormat::FORMAT_R32G32B32A32_FLOAT:		return "FORMAT_R32G32B32A32_FLOAT";
	case EFormat::FORMAT_R32G32B32A32_UINT:			return "FORMAT_R32G32B32A32_UINT";
	case EFormat::FORMAT_R32G32B32A32_SINT:			return "FORMAT_R32G32B32A32_SINT";
	case EFormat::FORMAT_R32G32B32_TYPELESS:		return "FORMAT_R32G32B32_TYPELESS";
	case EFormat::FORMAT_R32G32B32_FLOAT:			return "FORMAT_R32G32B32_FLOAT";
	case EFormat::FORMAT_R32G32B32_UINT:			return "FORMAT_R32G32B32_UINT";
	case EFormat::FORMAT_R32G32B32_SINT:			return "FORMAT_R32G32B32_SINT";
	case EFormat::FORMAT_R16G16B16A16_TYPELESS:		return "FORMAT_R16G16B16A16_TYPELESS";
	case EFormat::FORMAT_R16G16B16A16_FLOAT:		return "FORMAT_R16G16B16A16_FLOAT";
	case EFormat::FORMAT_R16G16B16A16_UNORM:		return "FORMAT_R16G16B16A16_UNORM";
	case EFormat::FORMAT_R16G16B16A16_UINT:			return "FORMAT_R16G16B16A16_UINT";
	case EFormat::FORMAT_R16G16B16A16_SNORM:		return "FORMAT_R16G16B16A16_SNORM";
	case EFormat::FORMAT_R16G16B16A16_SINT:			return "FORMAT_R16G16B16A16_SINT";
	case EFormat::FORMAT_R32G32_TYPELESS:			return "FORMAT_R32G32_TYPELESS";
	case EFormat::FORMAT_R32G32_FLOAT:				return "FORMAT_R32G32_FLOAT";
	case EFormat::FORMAT_R32G32_UINT:				return "FORMAT_R32G32_UINT";
	case EFormat::FORMAT_R32G32_SINT:				return "FORMAT_R32G32_SINT";
	case EFormat::FORMAT_R32G8X24_TYPELESS:			return "FORMAT_R32G8X24_TYPELESS";
	case EFormat::FORMAT_D32_FLOAT_S8X24_UINT:		return "FORMAT_D32_FLOAT_S8X24_UINT";
	case EFormat::FORMAT_R32_FLOAT_X8X24_TYPELESS:	return "FORMAT_R32_FLOAT_X8X24_TYPELESS";
	case EFormat::FORMAT_X32_TYPELESS_G8X24_UINT:	return "FORMAT_X32_TYPELESS_G8X24_UINT";
	case EFormat::FORMAT_R10G10B10A2_TYPELESS:		return "FORMAT_R10G10B10A2_TYPELESS";
	case EFormat::FORMAT_R10G10B10A2_UNORM:			return "FORMAT_R10G10B10A2_UNORM";
	case EFormat::FORMAT_R10G10B10A2_UINT:			return "FORMAT_R10G10B10A2_UINT";
	case EFormat::FORMAT_R11G11B10_FLOAT:			return "FORMAT_R11G11B10_FLOAT";
	case EFormat::FORMAT_R8G8B8A8_TYPELESS:			return "FORMAT_R8G8B8A8_TYPELESS";
	case EFormat::FORMAT_R8G8B8A8_UNORM:			return "FORMAT_R8G8B8A8_UNORM";
	case EFormat::FORMAT_R8G8B8A8_UNORM_SRGB:		return "FORMAT_R8G8B8A8_UNORM_SRGB";
	case EFormat::FORMAT_R8G8B8A8_UINT:				return "FORMAT_R8G8B8A8_UINT";
	case EFormat::FORMAT_R8G8B8A8_SNORM:			return "FORMAT_R8G8B8A8_SNORM";
	case EFormat::FORMAT_R8G8B8A8_SINT:				return "FORMAT_R8G8B8A8_SINT";
	case EFormat::FORMAT_R16G16_TYPELESS:			return "FORMAT_R16G16_TYPELESS";
	case EFormat::FORMAT_R16G16_FLOAT:				return "FORMAT_R16G16_FLOAT";
	case EFormat::FORMAT_R16G16_UNORM:				return "FORMAT_R16G16_UNORM";
	case EFormat::FORMAT_R16G16_UINT:				return "FORMAT_R16G16_UINT";
	case EFormat::FORMAT_R16G16_SNORM:				return "FORMAT_R16G16_SNORM";
	case EFormat::FORMAT_R16G16_SINT:				return "FORMAT_R16G16_SINT";
	case EFormat::FORMAT_R32_TYPELESS:				return "FORMAT_R32_TYPELESS";
	case EFormat::FORMAT_D32_FLOAT:					return "FORMAT_D32_FLOAT";
	case EFormat::FORMAT_R32_FLOAT:					return "FORMAT_R32_FLOAT";
	case EFormat::FORMAT_R32_UINT:					return "FORMAT_R32_UINT";
	case EFormat::FORMAT_R32_SINT:					return "FORMAT_R32_SINT";
	case EFormat::FORMAT_R24G8_TYPELESS:			return "FORMAT_R24G8_TYPELESS";
	case EFormat::FORMAT_D24_UNORM_S8_UINT:			return "FORMAT_D24_UNORM_S8_UINT";
	case EFormat::FORMAT_R24_UNORM_X8_TYPELESS:		return "FORMAT_R24_UNORM_X8_TYPELESS";
	case EFormat::FORMAT_X24_TYPELESS_G8_UINT:		return "FORMAT_X24_TYPELESS_G8_UINT";
	case EFormat::FORMAT_R8G8_TYPELESS:				return "FORMAT_R8G8_TYPELESS";
	case EFormat::FORMAT_R8G8_UNORM:				return "FORMAT_R8G8_UNORM";
	case EFormat::FORMAT_R8G8_UINT:					return "FORMAT_R8G8_UINT";
	case EFormat::FORMAT_R8G8_SNORM:				return "FORMAT_R8G8_SNORM";
	case EFormat::FORMAT_R8G8_SINT:					return "FORMAT_R8G8_SINT";
	case EFormat::FORMAT_R16_TYPELESS:				return "FORMAT_R16_TYPELESS";
	case EFormat::FORMAT_R16_FLOAT:					return "FORMAT_R16_FLOAT";
	case EFormat::FORMAT_D16_UNORM:					return "FORMAT_D16_UNORM";
	case EFormat::FORMAT_R16_UNORM:					return "FORMAT_R16_UNORM";
	case EFormat::FORMAT_R16_UINT:					return "FORMAT_R16_UINT";
	case EFormat::FORMAT_R16_SNORM:					return "FORMAT_R16_SNORM";
	case EFormat::FORMAT_R16_SINT:					return "FORMAT_R16_SINT";
	case EFormat::FORMAT_R8_TYPELESS:				return "FORMAT_R8_TYPELESS";
	case EFormat::FORMAT_R8_UNORM:					return "FORMAT_R8_UNORM";
	case EFormat::FORMAT_R8_UINT:					return "FORMAT_R8_UINT";
	case EFormat::FORMAT_R8_SNORM:					return "FORMAT_R8_SNORM";
	case EFormat::FORMAT_R8_SINT:					return "FORMAT_R8_SINT";
	case EFormat::FORMAT_A8_UNORM:					return "FORMAT_A8_UNORM";
	case EFormat::FORMAT_R1_UNORM:					return "FORMAT_R1_UNORM";
	case EFormat::FORMAT_B5G6R5_UNORM:				return "FORMAT_B5G6R5_UNORM";
	case EFormat::FORMAT_B5G5R5A1_UNORM:			return "FORMAT_B5G5R5A1_UNORM";
	case EFormat::FORMAT_B8G8R8A8_UNORM:			return "FORMAT_B8G8R8A8_UNORM";
	case EFormat::FORMAT_B8G8R8X8_UNORM:			return "FORMAT_B8G8R8X8_UNORM";
	case EFormat::FORMAT_B8G8R8A8_TYPELESS:			return "FORMAT_B8G8R8A8_TYPELESS";
	case EFormat::FORMAT_B8G8R8A8_UNORM_SRGB:		return "FORMAT_B8G8R8A8_UNORM_SRGB";
	case EFormat::FORMAT_B8G8R8X8_TYPELESS:			return "FORMAT_B8G8R8X8_TYPELESS";
	case EFormat::FORMAT_B8G8R8X8_UNORM_SRGB:		return "FORMAT_B8G8R8X8_UNORM_SRGB";
	default:										return "FORMAT_UNKNOWN";
	}
}

/*
* EMemoryType
*/

enum class EMemoryType
{
	MEMORY_TYPE_CPU_VISIBLE	= 0,
	MEMORY_TYPE_GPU			= 1,
};

/*
* Color
*/

struct ColorClearValue
{
	inline ColorClearValue()
		: R(1.0f)
		, G(1.0f)
		, B(1.0f)
		, A(1.0f)
	{
	}

	inline ColorClearValue(Float32 InR, Float32 InG, Float32 InB, Float32 InA)
		: R(InR)
		, G(InG)
		, B(InB)
		, A(InA)
	{
	}

	union
	{
		struct 
		{
			Float32 R;
			Float32 G;
			Float32 B;
			Float32 A;
		};

		Float32 ColorArr[4];
	};
};

/*
* DepthStencilClearValue
*/

struct DepthStencilClearValue
{
	inline DepthStencilClearValue()
		: Depth(1.0f)
		, Stencil(0)
	{
	}

	inline DepthStencilClearValue(Float32 InDepth, Uint8 InStencil)
		: Depth(InDepth)
		, Stencil(InStencil)
	{
	}

	Float32 Depth;
	Uint8	Stencil;
};

/*
* ClearValue
*/

struct ClearValue
{
	inline ClearValue()
		: Color()
		, HasClearColor(true)
	{
	}

	inline ClearValue(const ColorClearValue& InClearColor)
		: Color(InClearColor)
		, HasClearColor(true)
	{
	}

	inline ClearValue(const DepthStencilClearValue& InDepthStencil)
		: DepthStencil(InDepthStencil)
		, HasClearColor(false)
	{
	}

	inline ClearValue(const ClearValue& Other)
		: Color()
		, HasClearColor(Other.HasClearColor)
	{
		if (HasClearColor)
		{
			Color = Other.Color;
		}
		else
		{
			DepthStencil = Other.DepthStencil;
		}
	}

	inline ClearValue& operator=(const ClearValue& Other)
	{
		HasClearColor = Other.HasClearColor;
		if (HasClearColor)
		{
			Color = Other.Color;
		}
		else
		{
			DepthStencil = Other.DepthStencil;
		}

		return *this;
	}

	inline ClearValue& operator=(const ColorClearValue& InColor)
	{
		HasClearColor	= true;
		Color			= InColor;
		return *this;
	}

	inline ClearValue& operator=(const DepthStencilClearValue& InDepthStencil)
	{
		HasClearColor	= false;
		DepthStencil	= InDepthStencil;
		return *this;
	}

	union
	{
		ColorClearValue Color;
		DepthStencilClearValue DepthStencil;
	};

	Bool HasClearColor;
};

/*
* Viewport
*/

struct Viewport
{
	inline Viewport()
		: Width(0.0f)
		, Height(0.0f)
		, MinDepth(0.0f)
		, MaxDepth(0.0f)
		, x(0.0f)
		, y(0.0f)
	{
	}

	inline Viewport(Float32 InWidth, Float32 InHeight, Float32 InMinDepth, Float32 InMaxDepth, Float32 InX, Float32 InY)
		: Width(InWidth)
		, Height(InHeight)
		, MinDepth(InMinDepth)
		, MaxDepth(InMaxDepth)
		, x(InX)
		, y(InY)
	{
	}

	Float32 Width;
	Float32 Height;
	Float32 MinDepth;
	Float32 MaxDepth;
	Float32 x;
	Float32 y;
};

/*
* ScissorRect
*/

struct ScissorRect
{
	inline ScissorRect()
		: Width(0.0f)
		, Height(0.0f)
		, x(0.0f)
		, y(0.0f)
	{
	}

	inline ScissorRect(Float32 InWidth, Float32 InHeight, Float32 InX, Float32 InY)
		: Width(InWidth)
		, Height(InHeight)
		, x(InX)
		, y(InY)
	{
	}

	Float32 Width;
	Float32 Height;
	Float32 x;
	Float32 y;
};

/*
* CopyBufferInfo
*/

struct CopyBufferInfo
{
	inline CopyBufferInfo()
		: SourceOffset(0)
		, DestinationOffset(0)
		, SizeInBytes(0)
	{
	}

	inline CopyBufferInfo(Uint64 InSourceOffset, Uint32 InDestinationOffset, Uint32 InSizeInBytes)
		: SourceOffset(InSourceOffset)
		, DestinationOffset(InDestinationOffset)
		, SizeInBytes(InSizeInBytes)
	{
	}

	Uint64 SourceOffset;
	Uint64 DestinationOffset;
	Uint64 SizeInBytes;
};