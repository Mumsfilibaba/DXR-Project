#pragma once
#include "Format.h"

/*
* EComparisonFunc
*/

enum class EComparisonFunc
{
	ComparisonFunc_Never		= 1,
	ComparisonFunc_Less			= 2,
	ComparisonFunc_Equal		= 3,
	ComparisonFunc_LessEqual	= 4,
	ComparisonFunc_Greater		= 5,
	ComparisonFunc_NotEqual		= 6,
	ComparisonFunc_GreaterEqual	= 7,
	ComparisonFunc_Always		= 8
};

inline const Char* ToString(EComparisonFunc ComparisonFunc)
{
	switch (ComparisonFunc)
	{
	case EComparisonFunc::ComparisonFunc_Never:			return "ComparisonFunc_Never";
	case EComparisonFunc::ComparisonFunc_Less:			return "ComparisonFunc_Less";
	case EComparisonFunc::ComparisonFunc_Equal:			return "ComparisonFunc_Equal";
	case EComparisonFunc::ComparisonFunc_LessEqual:		return "ComparisonFunc_LessEqual";
	case EComparisonFunc::ComparisonFunc_Greater:		return "ComparisonFunc_Greater";
	case EComparisonFunc::ComparisonFunc_NotEqual:		return "ComparisonFunc_NotEqual";
	case EComparisonFunc::ComparisonFunc_GreaterEqual:	return "ComparisonFunc_GreaterEqual";
	case EComparisonFunc::ComparisonFunc_Always:		return "ComparisonFunc_Always";
	default:											return "";
	}
}

/*
* EPrimitiveTopologyType
*/

enum class EPrimitiveTopologyType
{
	PrimitiveTopologyType_Undefined	= 0,
	PrimitiveTopologyType_Point		= 1,
	PrimitiveTopologyType_Line		= 2,
	PrimitiveTopologyType_Triangle	= 3,
	PrimitiveTopologyType_Patch		= 4
};

inline const Char* ToString(EPrimitiveTopologyType PrimitveTopologyType)
{
	switch (PrimitveTopologyType)
	{
	case EPrimitiveTopologyType::PrimitiveTopologyType_Undefined:	return "PrimitiveTopologyType_Undefined";
	case EPrimitiveTopologyType::PrimitiveTopologyType_Point:		return "PrimitiveTopologyType_Point";
	case EPrimitiveTopologyType::PrimitiveTopologyType_Line:		return "PrimitiveTopologyType_Line";
	case EPrimitiveTopologyType::PrimitiveTopologyType_Triangle:	return "PrimitiveTopologyType_Triangle";
	case EPrimitiveTopologyType::PrimitiveTopologyType_Patch:		return "PrimitiveTopologyType_Patch";
	default:														return "";
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
* EPrimitiveTopology
*/

enum EPrimitiveTopology
{
	PrimitiveTopology_Undefined		= 0,
	PrimitiveTopology_PointList		= 1,
	PrimitiveTopology_LineList		= 2,
	PrimitiveTopology_LineStrip		= 3,
	PrimitiveTopology_TriangleList	= 4,
	PrimitiveTopology_TriangleStrip	= 5,
};

inline const Char* ToString(EPrimitiveTopology ResourceState)
{
	switch (ResourceState)
	{
	case EPrimitiveTopology::PrimitiveTopology_Undefined:		return "PrimitiveTopology_Undefined";
	case EPrimitiveTopology::PrimitiveTopology_PointList:		return "PrimitiveTopology_PointList";
	case EPrimitiveTopology::PrimitiveTopology_LineList:		return "PrimitiveTopology_LineList";
	case EPrimitiveTopology::PrimitiveTopology_LineStrip:		return "PrimitiveTopology_LineStrip";
	case EPrimitiveTopology::PrimitiveTopology_TriangleList:	return "PrimitiveTopology_TriangleList";
	case EPrimitiveTopology::PrimitiveTopology_TriangleStrip:	return "PrimitiveTopology_TriangleStrip";
	default:													return "";
	}
}

/*
* ColorClearValue
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

	inline ColorClearValue(Float InR, Float InG, Float InB, Float InA)
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
			Float R;
			Float G;
			Float B;
			Float A;
		};

		Float ColorArr[4];
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

	inline DepthStencilClearValue(Float InDepth, UInt8 InStencil)
		: Depth(InDepth)
		, Stencil(InStencil)
	{
	}

	Float Depth;
	UInt8 Stencil;
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

	inline Viewport(Float InWidth, Float InHeight, Float InMinDepth, Float InMaxDepth, Float InX, Float InY)
		: Width(InWidth)
		, Height(InHeight)
		, MinDepth(InMinDepth)
		, MaxDepth(InMaxDepth)
		, x(InX)
		, y(InY)
	{
	}

	Float Width;
	Float Height;
	Float MinDepth;
	Float MaxDepth;
	Float x;
	Float y;
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

	inline ScissorRect(Float InWidth, Float InHeight, Float InX, Float InY)
		: Width(InWidth)
		, Height(InHeight)
		, x(InX)
		, y(InY)
	{
	}

	Float Width;
	Float Height;
	Float x;
	Float y;
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

	inline CopyBufferInfo(UInt64 InSourceOffset, UInt32 InDestinationOffset, UInt32 InSizeInBytes)
		: SourceOffset(InSourceOffset)
		, DestinationOffset(InDestinationOffset)
		, SizeInBytes(InSizeInBytes)
	{
	}

	UInt64 SourceOffset;
	UInt32 DestinationOffset;
	UInt32 SizeInBytes;
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

	inline CopyTextureInfo(UInt64 InSourceOffset, UInt32 InDestinationOffset, UInt32 InSizeInBytes)
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

	UInt32 SourceX;
	UInt32 SourceY;
	UInt32 SourceZ;
	UInt32 DestX;
	UInt32 DestY;
	UInt32 DestZ;
	UInt32 Width;
	UInt32 Height;
	UInt32 Depth;
};

