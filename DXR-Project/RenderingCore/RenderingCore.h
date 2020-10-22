#pragma once
#include "Format.h"

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
	PrimitveTopologyType_Undefined	= 0,
	PrimitveTopologyType_Point		= 1,
	PrimitveTopologyType_Line		= 2,
	PrimitveTopologyType_Triangle	= 3,
	PrimitveTopologyType_Patch		= 4
};

inline const Char* ToString(EPrimitveTopologyType PrimitveTopologyType)
{
	switch (PrimitveTopologyType)
	{
	case EPrimitveTopologyType::PrimitveTopologyType_Undefined:	return "PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED";
	case EPrimitveTopologyType::PrimitveTopologyType_Point:		return "PRIMITIVE_TOPOLOGY_TYPE_POINT";
	case EPrimitveTopologyType::PrimitveTopologyType_Line:		return "PRIMITIVE_TOPOLOGY_TYPE_LINE";
	case EPrimitveTopologyType::PrimitveTopologyType_Triangle:	return "PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE";
	case EPrimitveTopologyType::PrimitveTopologyType_Patch:		return "PRIMITIVE_TOPOLOGY_TYPE_PATCH";
	default:													return "";
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
* EResourceState
*/

enum class EResourceState
{
	ResourceState_Common							= 0,
	ResourceState_VertexAndConstantBuffer			= 1,
	ResourceState_IndexBuffer						= 2,
	ResourceState_RenderTarget						= 3,
	ResourceState_UnorderedAccess					= 4,
	ResourceState_DepthWrite						= 5,
	ResourceState_DepthRead							= 6,
	ResourceState_NonPixelShaderResource			= 7,
	ResourceState_PixelShaderResource				= 8,
	ResourceState_CopyDest							= 9,
	ResourceState_CopySource						= 10,
	ResourceState_ResolveDest						= 11,
	ResourceState_ResolveSource						= 12,
	ResourceState_RayTracingAccelerationStructure	= 13,
	ResourceState_ShadingRateSource					= 14,
	ResourceState_Present							= 15,
};

inline const Char* ToString(EResourceState ResourceState)
{
	switch (ResourceState)
	{
	case EResourceState::ResourceState_Common:							return "ResourceState_Common";
	case EResourceState::ResourceState_VertexAndConstantBuffer:			return "ResourceState_VertexAndConstantBuffer";
	case EResourceState::ResourceState_IndexBuffer:						return "ResourceState_IndexBuffer";
	case EResourceState::ResourceState_RenderTarget:					return "ResourceState_RenderTarget";
	case EResourceState::ResourceState_UnorderedAccess:					return "ResourceState_UnorderedAccess";
	case EResourceState::ResourceState_DepthWrite:						return "ResourceState_DepthWrite";
	case EResourceState::ResourceState_DepthRead:						return "ResourceState_DepthRead";
	case EResourceState::ResourceState_NonPixelShaderResource:			return "ResourceState_NonPixelShaderResource";
	case EResourceState::ResourceState_PixelShaderResource:				return "ResourceState_PixelShaderResource";
	case EResourceState::ResourceState_CopyDest:						return "ResourceState_CopyDest";
	case EResourceState::ResourceState_CopySource:						return "ResourceState_CopySource";
	case EResourceState::ResourceState_ResolveDest:						return "ResourceState_ResolveDest";
	case EResourceState::ResourceState_ResolveSource:					return "ResourceState_ResolveSource";
	case EResourceState::ResourceState_RayTracingAccelerationStructure:	return "ResourceState_RayTracingAccelerationStructure";
	case EResourceState::ResourceState_ShadingRateSource:				return "ResourceState_ShadingRateSource";
	case EResourceState::ResourceState_Present:							return "ResourceState_Present";
	default:															return "";
	}
}

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

/*
* CopyTextureInfo
*/

// TODO: This will not work, fix
struct CopyTextureInfo
{
	inline CopyTextureInfo()
		: SourceX(0)
		, SourceY(0)
		, SourceZ(0)
		, DestX(0)
		, DestY(0)
		, DestZ(0)
		, Width(0)
		, Height(0)
		, Depth(0)
	{
	}

	inline CopyTextureInfo(Uint64 InSourceOffset, Uint32 InDestinationOffset, Uint32 InSizeInBytes)
		: SourceX(0)
		, SourceY(0)
		, SourceZ(0)
		, DestX(0)
		, DestY(0)
		, DestZ(0)
		, Width(0)
		, Height(0)
		, Depth(0)
	{
	}

	Uint32 SourceX;
	Uint32 SourceY;
	Uint32 SourceZ;
	Uint32 DestX;
	Uint32 DestY;
	Uint32 DestZ;
	Uint32 Width;
	Uint32 Height;
	Uint32 Depth;
};

