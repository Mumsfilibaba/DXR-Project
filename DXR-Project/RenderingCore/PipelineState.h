#pragma once
#include "Resource.h"

class GraphicsPipelineState;
class ComputePipelineState;
class RayTracingPipelineState;
class ComputeShader;
class VertexShader;
class PixelShader;

/*
* PipelineState
*/

class PipelineState : public PipelineResource
{
public:
	PipelineState() = default;
	~PipelineState() = default;

	virtual GraphicsPipelineState* AsGraphics()
	{
		return nullptr;
	}

	virtual const GraphicsPipelineState* AsGraphics() const
	{
		return nullptr;
	}

	virtual ComputePipelineState* AsCompute() 
	{
		return nullptr;
	}
	
	virtual const ComputePipelineState* AsCompute() const 
	{
		return nullptr;
	}

	virtual RayTracingPipelineState* AsRayTracing() 
	{
		return nullptr;
	}

	virtual const RayTracingPipelineState* AsRayTracing() const 
	{
		return nullptr;
	}
};

/*
* EDepthWriteMask
*/

enum class EDepthWriteMask
{
	DepthWriteMask_Zero	= 0,
	DepthWriteMask_All	= 1
};

inline const Char* ToString(EDepthWriteMask DepthWriteMask)
{
	switch (DepthWriteMask)
	{
	case EDepthWriteMask::DepthWriteMask_Zero:	return "DepthWriteMask_Zero";
	case EDepthWriteMask::DepthWriteMask_All:	return "DepthWriteMask_All";
	default:									return "";
	}
}

/*
* EStencilOp
*/

enum class EStencilOp
{
	StencilOp_Keep		= 1,
	StencilOp_Zero		= 2,
	StencilOp_Replace	= 3,
	StencilOp_IncrSat	= 4,
	StencilOp_DecrSat	= 5,
	StencilOp_Invert	= 6,
	StencilOp_Incr		= 7,
	StencilOp_Decr		= 8
};

inline const Char* ToString(EStencilOp StencilOp)
{
	switch (StencilOp)
	{
	case EStencilOp::StencilOp_Keep:	return "StencilOp_Keep";
	case EStencilOp::StencilOp_Zero:	return "StencilOp_Zero";
	case EStencilOp::StencilOp_Replace:	return "StencilOp_Replace";
	case EStencilOp::StencilOp_IncrSat:	return "StencilOp_IncrSat";
	case EStencilOp::StencilOp_DecrSat:	return "StencilOp_DecrSat";
	case EStencilOp::StencilOp_Invert:	return "StencilOp_Invert";
	case EStencilOp::StencilOp_Incr:	return "StencilOp_Incr";
	case EStencilOp::StencilOp_Decr:	return "StencilOp_Decr";
	default:							return "";
	}
}

/*
* DepthStencilStateInitializer
*/

struct DepthStencilOp
{
	EStencilOp		StencilFailOp;
	EStencilOp		StencilDepthFailOp;
	EStencilOp		StencilPassOp;
	EComparisonFunc	StencilFunc;
};

/*
* DepthStencilStateInitializer
*/

struct DepthStencilStateInitalizer
{
	bool			DepthEnable;
	EDepthWriteMask	DepthWriteMask;
	EComparisonFunc	DepthFunc;
	bool			StencilEnable;
	Uint8			StencilReadMask;
	Uint8			StencilWriteMask;
	DepthStencilOp	FrontFace;
	DepthStencilOp	BackFace;
};

/*
* DepthStencilState
*/

class DepthStencilState : public PipelineResource
{
public:
	virtual Uint64 GetHash() = 0;
};

/*
* ECullMode
*/

enum class ECullMode
{
	CULL_MODE_NONE	= 1,
	CULL_MODE_FRONT	= 2,
	CULL_MODE_BACK	= 3
};

inline const Char* ToString(ECullMode CullMode)
{
	switch (CullMode)
	{
	case ECullMode::CULL_MODE_NONE:		return "CULL_MODE_NONE";
	case ECullMode::CULL_MODE_FRONT:	return "CULL_MODE_FRONT";
	case ECullMode::CULL_MODE_BACK:		return "CULL_MODE_BACK";
	default:							return "";
	}
}

/*
* EFillMode
*/

enum class EFillMode
{
	FILL_MODE_WIREFRAME	= 1,
	FILL_MODE_SOLID		= 2
};

inline const Char* ToString(EFillMode FillMode)
{
	switch (FillMode)
	{
	case EFillMode::FILL_MODE_WIREFRAME:	return "FILL_MODE_WIREFRAME";
	case EFillMode::FILL_MODE_SOLID:		return "FILL_MODE_SOLID";
	default:								return "";
	}
}

/*
* RasterizerStateInitializer
*/

struct RasterizerStateInitializer
{
	EFillMode	FillMode;
	ECullMode	CullMode;
	Bool		FrontCounterClockwise;
	Int32		DepthBias;
	Float32		DepthBiasClamp;
	Bool		SlopeScaledDepthBias;
	Bool		DepthClipEnable;
	Bool		MultisampleEnable;
	Bool		AntialiasedLineEnable;
	Uint32		ForcedSampleCount;
	Bool		EnableConservativeRaster;
};

/*
* RasterizerState
*/

class RasterizerState : public PipelineResource
{
public:
	virtual Uint64 GetHash() = 0;
};


/*
* EBlendFactor
*/

enum class EBlendFactor
{
	BLEND_FACTOR_ZERO				= 1,
	BLEND_FACTOR_ONE				= 2,
	BLEND_FACTOR_SRC_COLOR			= 3,
	BLEND_FACTOR_INV_SRC_COLOR		= 4,
	BLEND_FACTOR_SRC_ALPHA			= 5,
	BLEND_FACTOR_INV_SRC_ALPHA		= 6,
	BLEND_FACTOR_DEST_ALPHA			= 7,
	BLEND_FACTOR_INV_DEST_ALPHA		= 8,
	BLEND_FACTOR_DEST_COLOR			= 9,
	BLEND_FACTOR_INV_DEST_COLOR		= 10,
	BLEND_FACTOR_SRC_ALPHA_SAT		= 11,
	BLEND_FACTOR_BLEND_FACTOR		= 12,
	BLEND_FACTOR_INV_BLEND_FACTOR	= 13,
	BLEND_FACTOR_SRC1_COLOR			= 14,
	BLEND_FACTOR_INV_SRC1_COLOR		= 15,
	BLEND_FACTOR_SRC1_ALPHA			= 16,
	BLEND_FACTOR_INV_SRC1_ALPHA		= 17
};

inline const Char* ToString(EBlendFactor BlendFactor)
{
	switch (BlendFactor)
	{
	case EBlendFactor::BLEND_FACTOR_ZERO:				return "BLEND_FACTOR_ZERO";
	case EBlendFactor::BLEND_FACTOR_ONE:				return "BLEND_FACTOR_ONE";
	case EBlendFactor::BLEND_FACTOR_SRC_COLOR:			return "BLEND_FACTOR_SRC_COLOR";
	case EBlendFactor::BLEND_FACTOR_INV_SRC_COLOR:		return "BLEND_FACTOR_INV_SRC_COLOR";
	case EBlendFactor::BLEND_FACTOR_SRC_ALPHA:			return "BLEND_FACTOR_SRC_ALPHA";
	case EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA:		return "BLEND_FACTOR_INV_SRC_ALPHA";
	case EBlendFactor::BLEND_FACTOR_DEST_ALPHA:			return "BLEND_FACTOR_DEST_ALPHA";
	case EBlendFactor::BLEND_FACTOR_INV_DEST_ALPHA:		return "BLEND_FACTOR_INV_DEST_ALPHA";
	case EBlendFactor::BLEND_FACTOR_DEST_COLOR:			return "BLEND_FACTOR_DEST_COLOR";
	case EBlendFactor::BLEND_FACTOR_INV_DEST_COLOR:		return "BLEND_FACTOR_INV_DEST_COLOR";
	case EBlendFactor::BLEND_FACTOR_SRC_ALPHA_SAT:		return "BLEND_FACTOR_SRC_ALPHA_SAT";
	case EBlendFactor::BLEND_FACTOR_BLEND_FACTOR:		return "BLEND_FACTOR_BLEND_FACTOR";
	case EBlendFactor::BLEND_FACTOR_INV_BLEND_FACTOR:	return "BLEND_FACTOR_INV_BLEND_FACTOR";
	case EBlendFactor::BLEND_FACTOR_SRC1_COLOR:			return "BLEND_FACTOR_SRC1_COLOR";
	case EBlendFactor::BLEND_FACTOR_INV_SRC1_COLOR:		return "BLEND_FACTOR_INV_SRC1_COLOR";
	case EBlendFactor::BLEND_FACTOR_SRC1_ALPHA:			return "BLEND_FACTOR_SRC1_ALPHA";
	case EBlendFactor::BLEND_FACTOR_INV_SRC1_ALPHA:		return "BLEND_FACTOR_INV_SRC1_ALPHA";
	default:											return "";
	}
}

/*
* EBlendOp
*/

enum class EBlendOp
{
	BLEND_OP_ADD			= 1,
	BLEND_OP_SUBTRACT		= 2,
	BLEND_OP_REV_SUBTRACT	= 3,
	BLEND_OP_MIN			= 4,
	BLEND_OP_MAX			= 5
};

inline const Char* ToString(EBlendOp BlendOp)
{
	switch (BlendOp)
	{
	case EBlendOp::BLEND_OP_ADD:			return "BLEND_OP_ADD";
	case EBlendOp::BLEND_OP_SUBTRACT:		return "BLEND_OP_SUBTRACT";
	case EBlendOp::BLEND_OP_REV_SUBTRACT:	return "BLEND_OP_REV_SUBTRACT";
	case EBlendOp::BLEND_OP_MIN:			return "BLEND_OP_MIN";
	case EBlendOp::BLEND_OP_MAX:			return "BLEND_OP_MAX";
	default:								return "";
	}
}

/*
* ELogicOp
*/

enum class ELogicOp
{
	LOGIC_OP_CLEAR			= 0,
	LOGIC_OP_SET			= 1,
	LOGIC_OP_COPY			= 2,
	LOGIC_OP_COPY_INVERTED	= 3,
	LOGIC_OP_NOOP			= 4,
	LOGIC_OP_INVERT			= 5,
	LOGIC_OP_AND			= 6,
	LOGIC_OP_NAND			= 7,
	LOGIC_OP_OR				= 8,
	LOGIC_OP_NOR			= 9,
	LOGIC_OP_XOR			= 10,
	LOGIC_OP_EQUIV			= 11,
	LOGIC_OP_AND_REVERSE	= 12,
	LOGIC_OP_AND_INVERTED	= 13,
	LOGIC_OP_OR_REVERSE		= 14,
	LOGIC_OP_OR_INVERTED	= 15
};

inline const Char* ToString(ELogicOp LogicOp)
{
	switch (LogicOp)
	{
	case ELogicOp::LOGIC_OP_CLEAR:			return "LOGIC_OP_CLEAR";
	case ELogicOp::LOGIC_OP_SET:			return "LOGIC_OP_SET";
	case ELogicOp::LOGIC_OP_COPY:			return "LOGIC_OP_COPY";
	case ELogicOp::LOGIC_OP_COPY_INVERTED:	return "LOGIC_OP_COPY_INVERTED";
	case ELogicOp::LOGIC_OP_NOOP:			return "LOGIC_OP_NOOP";
	case ELogicOp::LOGIC_OP_INVERT:			return "LOGIC_OP_INVERT";
	case ELogicOp::LOGIC_OP_AND:			return "LOGIC_OP_AND";
	case ELogicOp::LOGIC_OP_NAND:			return "LOGIC_OP_NAND";
	case ELogicOp::LOGIC_OP_OR:				return "LOGIC_OP_OR";
	case ELogicOp::LOGIC_OP_NOR:			return "LOGIC_OP_NOR";
	case ELogicOp::LOGIC_OP_XOR:			return "LOGIC_OP_XOR";
	case ELogicOp::LOGIC_OP_EQUIV:			return "LOGIC_OP_EQUIV";
	case ELogicOp::LOGIC_OP_AND_REVERSE:	return "LOGIC_OP_AND_REVERSE";
	case ELogicOp::LOGIC_OP_AND_INVERTED:	return "LOGIC_OP_AND_INVERTED";
	case ELogicOp::LOGIC_OP_OR_REVERSE:		return "LOGIC_OP_OR_REVERSE";
	case ELogicOp::LOGIC_OP_OR_INVERTED:	return "LOGIC_OP_OR_INVERTED";
	default:								return "";
	}
}

/*
* EColorWriteFlag
*/

typedef Uint8 ColorWriteFlags;
enum EColorWriteFlag : ColorWriteFlags
{
	COLOR_WRITE_FLAG_NONE	= 0,
	COLOR_WRITE_FLAG_RED	= 1,
	COLOR_WRITE_FLAG_GREEN	= 2,
	COLOR_WRITE_FLAG_BLUE	= 4,
	COLOR_WRITE_FLAG_ALPHA	= 8,
	COLOR_WRITE_FLAG_ALL	= (((COLOR_WRITE_FLAG_RED | COLOR_WRITE_FLAG_GREEN) | COLOR_WRITE_FLAG_BLUE) | COLOR_WRITE_FLAG_ALPHA)
};

/*
* RenderTargetWriteState
*/

struct RenderTargetWriteState
{
	inline RenderTargetWriteState(ColorWriteFlags Flags)
		: Mask(Flags)
	{
	}

	inline bool WriteNone() const
	{
		return Mask == COLOR_WRITE_FLAG_NONE;
	}

	inline bool WriteRed() const
	{
		return (Mask & COLOR_WRITE_FLAG_RED);
	}

	inline bool WriteGreen() const
	{
		return (Mask & COLOR_WRITE_FLAG_GREEN);
	}

	inline bool WriteBlue() const
	{
		return (Mask & COLOR_WRITE_FLAG_BLUE);
	}

	inline bool WriteAlpha() const
	{
		return (Mask & COLOR_WRITE_FLAG_ALPHA);
	}

	inline bool WriteAll() const
	{
		return Mask == COLOR_WRITE_FLAG_ALL;
	}

	ColorWriteFlags Mask;
};

/*
* RenderTargetBlendState
*/

struct RenderTargetBlendState
{
	Bool					BlendEnable;
	Bool					LogicOpEnable;
	EBlendFactor			SrcBlend;
	EBlendFactor			DestBlend;
	EBlendOp				BlendOp;
	EBlendFactor			SrcBlendAlpha;
	EBlendFactor			DestBlendAlpha;
	EBlendOp				BlendOpAlpha;
	ELogicOp				LogicOp;
	RenderTargetWriteState	RenderTargetWriteMask;
};

/*
* BlendStateInitializer
*/

struct BlendStateInitializer 
{
	Bool					AlphaToCoverageEnable;
	Bool					IndependentBlendEnable;
	RenderTargetBlendState	RenderTarget[8];
};

/*
* BlendState
*/

class BlendState : public PipelineResource
{
public:
	virtual Uint64 GetHash() const = 0;
};

/*
* InputLayout
*/

class InputLayout : public PipelineResource
{
public:
	virtual Uint64 GetHash() const = 0;
};

/*
* EIndexBufferStripCutValue
*/

enum class EIndexBufferStripCutValue
{
	INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED		= 0,
	INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF			= 1,
	INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF		= 2
};

inline const Char* ToString(EIndexBufferStripCutValue IndexBufferStripCutValue)
{
	switch (IndexBufferStripCutValue)
	{
	case EIndexBufferStripCutValue::INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED:		return "INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED";
	case EIndexBufferStripCutValue::INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF:		return "INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF";
	case EIndexBufferStripCutValue::INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF:	return "INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF";
	default:																	return "";
	}
}

/*
* PipelineRenderTargetFormats
*/

struct PipelineRenderTargetFormats
{
	inline PipelineRenderTargetFormats()
		: RenderTargetFormats()
		, NumRenderTargets(1)
		, DepthStencilFormat(EFormat::Format_D24_Unorm_S8_Uint)
	{
	}

	EFormat RenderTargetFormats[8];
	Uint32	NumRenderTargets;
	EFormat DepthStencilFormat;
};

/*
* PipelineShaderState
*/

struct PipelineShaderState
{
	inline PipelineShaderState()
		: VertexShader(nullptr)
		, PixelShader(nullptr)
	{
	}

	inline PipelineShaderState(VertexShader* InVertexShader, PixelShader* InPixelShader)
		: VertexShader(InVertexShader)
		, PixelShader(InPixelShader)
	{
	}

	VertexShader*	VertexShader;
	PixelShader*	PixelShader;
};

/*
* GraphicsPipelineStateInitliazer
*/

struct GraphicsPipelineStateInitliazer
{
	PipelineShaderState			ShaderState;
	BlendState*					BlendState;
	InputLayout*				InputLayout;
	RasterizerState*			RasterizerState;
	Uint32						SampleMask;
	DepthStencilState*			DepthStencilState;
	EIndexBufferStripCutValue	IBStripCutValue;
	EPrimitiveTopologyType		PrimitiveTopologyType;
	PipelineRenderTargetFormats PipelineFormats;
};

/*
* GraphicsPipelineState
*/

class GraphicsPipelineState : public PipelineState
{
public:
	GraphicsPipelineState() = default;
	~GraphicsPipelineState() = default;

	virtual bool Initialize(const GraphicsPipelineStateInitliazer& Initalizer) = 0;

	virtual GraphicsPipelineState* AsGraphics() override 
	{
		return this;
	}

	virtual const GraphicsPipelineState* AsGraphics() const override
	{
		return this;
	}
};

/*
* ComputePipelineStateCreateInfo
*/

struct ComputePipelineStateCreateInfo
{
	inline ComputePipelineStateCreateInfo()
		: Shader(nullptr)
	{
	}

	inline ComputePipelineStateCreateInfo(ComputeShader* InShader)
		: Shader(InShader)
	{
	}

	ComputeShader* Shader;
};

/*
* ComputePipelineState
*/

class ComputePipelineState : public PipelineState
{
public:
	ComputePipelineState() = default;
	~ComputePipelineState() = default;

	virtual ComputePipelineState* AsCompute() override
	{
		return this;
	}

	virtual const ComputePipelineState* AsCompute() const override
	{
		return this;
	}
};

/*
* RayTracingPipelineState
*/

class RayTracingPipelineState : public PipelineState
{
public:
	RayTracingPipelineState() = default;
	~RayTracingPipelineState() = default;

	virtual bool Initialize() = 0;

	virtual RayTracingPipelineState* AsRayTracing() override
	{
		return this;
	}

	virtual const RayTracingPipelineState* AsRayTracing() const override
	{
		return this;
	}
};