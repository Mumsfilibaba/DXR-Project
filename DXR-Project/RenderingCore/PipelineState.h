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
	CullMode_None	= 1,
	CullMode_Front	= 2,
	CullMode_Back	= 3
};

inline const Char* ToString(ECullMode CullMode)
{
	switch (CullMode)
	{
	case ECullMode::CullMode_None:	return "CullMode_None";
	case ECullMode::CullMode_Front:	return "CullMode_Front";
	case ECullMode::CullMode_Back:	return "CullMode_Back";
	default:						return "";
	}
}

/*
* EFillMode
*/

enum class EFillMode
{
	FillMode_WireFrame	= 1,
	FillMode_Solid		= 2
};

inline const Char* ToString(EFillMode FillMode)
{
	switch (FillMode)
	{
	case EFillMode::FillMode_WireFrame:	return "FillMode_WireFrame";
	case EFillMode::FillMode_Solid:		return "FillMode_Solid";
	default:							return "";
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
};


/*
* EBlendFactor
*/

enum class EBlendFactor
{
	BlendFactor_Zero			= 1,
	BlendFactor_One				= 2,
	BlendFactor_SrcColor		= 3,
	BlendFactor_InvSrcColor		= 4,
	BlendFactor_SrcAlpha		= 5,
	BlendFactor_InvSrcAlpha		= 6,
	BlendFactor_DestAlpha		= 7,
	BlendFactor_InvDestAlpha	= 8,
	BlendFactor_DestColor		= 9,
	BlendFactor_InvDestColor	= 10,
	BlendFactor_SrcAlphaSat		= 11,
	BlendFactor_BlendFactor		= 12,
	BlendFactor_InvBlendFactor	= 13,
	BlendFactor_Src1Color		= 14,
	BlendFactor_InvSrc1Color	= 15,
	BlendFactor_Src1Alpha		= 16,
	BlendFactor_InvSrc1Alpha	= 17
};

inline const Char* ToString(EBlendFactor BlendFactor)
{
	switch (BlendFactor)
	{
	case EBlendFactor::BlendFactor_Zero:			return "BlendFactor_Zero";
	case EBlendFactor::BlendFactor_One:				return "BlendFactor_One";
	case EBlendFactor::BlendFactor_SrcColor:		return "BlendFactor_SrcColor";
	case EBlendFactor::BlendFactor_InvSrcColor:		return "BlendFactor_InvSrcColor";
	case EBlendFactor::BlendFactor_SrcAlpha:		return "BlendFactor_SrcAlpha";
	case EBlendFactor::BlendFactor_InvSrcAlpha:		return "BlendFactor_InvSrcAlpha";
	case EBlendFactor::BlendFactor_DestAlpha:		return "BlendFactor_DestAlpha";
	case EBlendFactor::BlendFactor_InvDestAlpha:	return "BlendFactor_InvDestAlpha";
	case EBlendFactor::BlendFactor_DestColor:		return "BlendFactor_DestColor";
	case EBlendFactor::BlendFactor_InvDestColor:	return "BlendFactor_InvDestColor";
	case EBlendFactor::BlendFactor_SrcAlphaSat:		return "BlendFactor_SrcAlphaSat";
	case EBlendFactor::BlendFactor_BlendFactor:		return "BlendFactor_BlendFactor";
	case EBlendFactor::BlendFactor_InvBlendFactor:	return "BlendFactor_InvBlendFactor";
	case EBlendFactor::BlendFactor_Src1Color:		return "BlendFactor_Src1Color";
	case EBlendFactor::BlendFactor_InvSrc1Color:	return "BlendFactor_InvSrc1Color";
	case EBlendFactor::BlendFactor_Src1Alpha:		return "BlendFactor_Src1Alpha";
	case EBlendFactor::BlendFactor_InvSrc1Alpha:	return "BlendFactor_InvSrc1Alpha";
	default:										return "";
	}
}

/*
* EBlendOp
*/

enum class EBlendOp
{
	BlendOp_Add			= 1,
	BlendOp_Subtract	= 2,
	BlendOp_RevSubtract	= 3,
	BlendOp_Min			= 4,
	BlendOp_Max			= 5
};

inline const Char* ToString(EBlendOp BlendOp)
{
	switch (BlendOp)
	{
	case EBlendOp::BlendOp_Add:			return "BlendOp_Add";
	case EBlendOp::BlendOp_Subtract:	return "BlendOp_Subtract";
	case EBlendOp::BlendOp_RevSubtract:	return "BlendOp_RevSubtract";
	case EBlendOp::BlendOp_Min:			return "BlendOp_Min";
	case EBlendOp::BlendOp_Max:			return "BlendOp_Max";
	default:							return "";
	}
}

/*
* ELogicOp
*/

enum class ELogicOp
{
	LogicOp_Clear			= 0,
	LogicOp_Set				= 1,
	LogicOp_Copy			= 2,
	LogicOp_CopyInverted	= 3,
	LogicOp_Noop			= 4,
	LogicOp_Invert			= 5,
	LogicOp_And				= 6,
	LogicOp_Nand			= 7,
	LogicOp_Or				= 8,
	LogicOp_Nor				= 9,
	LogicOp_Xor				= 10,
	LogicOp_Equiv			= 11,
	LogicOp_AndReverse		= 12,
	LogicOp_AndInverted		= 13,
	LogicOp_OrReverse		= 14,
	LogicOp_OrInverted		= 15
};

inline const Char* ToString(ELogicOp LogicOp)
{
	switch (LogicOp)
	{
	case ELogicOp::LogicOp_Clear:			return "LogicOp_Clear";
	case ELogicOp::LogicOp_Set:				return "LogicOp_Set";
	case ELogicOp::LogicOp_Copy:			return "LogicOp_Copy";
	case ELogicOp::LogicOp_CopyInverted:	return "LogicOp_CopyInverted";
	case ELogicOp::LogicOp_Noop:			return "LogicOp_Noop";
	case ELogicOp::LogicOp_Invert:			return "LogicOp_Invert";
	case ELogicOp::LogicOp_And:				return "LogicOp_And";
	case ELogicOp::LogicOp_Nand:			return "LogicOp_Nand";
	case ELogicOp::LogicOp_Or:				return "LogicOp_Or";
	case ELogicOp::LogicOp_Nor:				return "LogicOp_Nor";
	case ELogicOp::LogicOp_Xor:				return "LogicOp_Xor";
	case ELogicOp::LogicOp_Equiv:			return "LogicOp_Equiv";
	case ELogicOp::LogicOp_AndReverse:		return "LogicOp_AndReverse";
	case ELogicOp::LogicOp_AndInverted:		return "LogicOp_AndInverted";
	case ELogicOp::LogicOp_OrReverse:		return "LogicOp_OrReverse";
	case ELogicOp::LogicOp_OrInverted:		return "LogicOp_OrInverted";
	default:								return "";
	}
}

/*
* EColorWriteFlag
*/

typedef Uint8 ColorWriteFlags;
enum EColorWriteFlag : ColorWriteFlags
{
	ColorWriteFlag_None		= 0,
	ColorWriteFlag_Red		= 1,
	ColorWriteFlag_Green	= 2,
	ColorWriteFlag_Blue		= 4,
	ColorWriteFlag_Alpha	= 8,
	ColorWriteFlag_All		= (((ColorWriteFlag_Red | ColorWriteFlag_Green) | ColorWriteFlag_Blue) | ColorWriteFlag_Alpha)
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
		return Mask == ColorWriteFlag_None;
	}

	inline bool WriteRed() const
	{
		return (Mask & ColorWriteFlag_Red);
	}

	inline bool WriteGreen() const
	{
		return (Mask & ColorWriteFlag_Green);
	}

	inline bool WriteBlue() const
	{
		return (Mask & ColorWriteFlag_Blue);
	}

	inline bool WriteAlpha() const
	{
		return (Mask & ColorWriteFlag_Alpha);
	}

	inline bool WriteAll() const
	{
		return Mask == ColorWriteFlag_All;
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
};

/*
* InputLayout
*/

class InputLayout : public PipelineResource
{
};

/*
* EIndexBufferStripCutValue
*/

enum class EIndexBufferStripCutValue
{
	IndexBufferStripCutValue_Disabled	= 0,
	IndexBufferStripCutValue_0xffff		= 1,
	IndexBufferStripCutValue_0xffffffff	= 2
};

inline const Char* ToString(EIndexBufferStripCutValue IndexBufferStripCutValue)
{
	switch (IndexBufferStripCutValue)
	{
	case EIndexBufferStripCutValue::IndexBufferStripCutValue_Disabled:		return "IndexBufferStripCutValue_Disabled";
	case EIndexBufferStripCutValue::IndexBufferStripCutValue_0xffff:		return "IndexBufferStripCutValue_0xffff";
	case EIndexBufferStripCutValue::IndexBufferStripCutValue_0xffffffff:	return "IndexBufferStripCutValue_0xffffffff";
	default:																return "";
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