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
	PipelineState()		= default;
	~PipelineState()	= default;

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
* DepthStencilOp
*/

struct DepthStencilOp
{
	EStencilOp		StencilFailOp		= EStencilOp::StencilOp_Keep;
	EStencilOp		StencilDepthFailOp	= EStencilOp::StencilOp_Keep;
	EStencilOp		StencilPassOp		= EStencilOp::StencilOp_Keep;
	EComparisonFunc	StencilFunc			= EComparisonFunc::ComparisonFunc_Always;
};

/*
* DepthStencilStateCreateInfo
*/

struct DepthStencilStateCreateInfo
{
	EDepthWriteMask	DepthWriteMask = EDepthWriteMask::DepthWriteMask_All;
	EComparisonFunc	DepthFunc = EComparisonFunc::ComparisonFunc_Less;
	bool			DepthEnable = true;
	Uint8			StencilReadMask = 0xff;
	Uint8			StencilWriteMask = 0xff;
	bool			StencilEnable = false;
	DepthStencilOp	FrontFace	= DepthStencilOp();
	DepthStencilOp	BackFace	= DepthStencilOp();
};

/*
* DepthStencilState
*/

class DepthStencilState : public PipelineResource
{
public:
	DepthStencilState()		= default;
	~DepthStencilState()	= default;
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
* RasterizerStateCreateInfo
*/

struct RasterizerStateCreateInfo
{
	EFillMode	FillMode = EFillMode::FillMode_Solid;
	ECullMode	CullMode = ECullMode::CullMode_Back;
	Bool		FrontCounterClockwise = false;
	Int32		DepthBias = 0;
	Float32		DepthBiasClamp = 0.0f;
	Float32		SlopeScaledDepthBias = 0.0f;
	Bool		DepthClipEnable = true;
	Bool		MultisampleEnable = false;
	Bool		AntialiasedLineEnable = false;
	Uint32		ForcedSampleCount = 0;
	Bool		EnableConservativeRaster = false;
};

/*
* RasterizerState
*/

class RasterizerState : public PipelineResource
{
public:
	RasterizerState()	= default;
	~RasterizerState()	= default;
};

/*
* EBlend
*/

enum class EBlend
{
	Blend_Zero				= 1,
	Blend_One				= 2,
	Blend_SrcColor			= 3,
	Blend_InvSrcColor		= 4,
	Blend_SrcAlpha			= 5,
	Blend_InvSrcAlpha		= 6,
	Blend_DestAlpha			= 7,
	Blend_InvDestAlpha		= 8,
	Blend_DestColor			= 9,
	Blend_InvDestColor		= 10,
	Blend_SrcAlphaSat		= 11,
	Blend_BlendFactor		= 12,
	Blend_InvBlendFactor	= 13,
	Blend_Src1Color			= 14,
	Blend_InvSrc1Color		= 15,
	Blend_Src1Alpha			= 16,
	Blend_InvSrc1Alpha		= 17
};

inline const Char* ToString(EBlend Blend)
{
	switch (Blend)
	{
	case EBlend::Blend_Zero:			return "Blend_Zero";
	case EBlend::Blend_One:				return "Blend_One";
	case EBlend::Blend_SrcColor:		return "Blend_SrcColor";
	case EBlend::Blend_InvSrcColor:		return "Blend_InvSrcColor";
	case EBlend::Blend_SrcAlpha:		return "Blend_SrcAlpha";
	case EBlend::Blend_InvSrcAlpha:		return "Blend_InvSrcAlpha";
	case EBlend::Blend_DestAlpha:		return "Blend_DestAlpha";
	case EBlend::Blend_InvDestAlpha:	return "Blend_InvDestAlpha";
	case EBlend::Blend_DestColor:		return "Blend_DestColor";
	case EBlend::Blend_InvDestColor:	return "Blend_InvDestColor";
	case EBlend::Blend_SrcAlphaSat:		return "Blend_SrcAlphaSat";
	case EBlend::Blend_BlendFactor:		return "Blend_BlendFactor";
	case EBlend::Blend_InvBlendFactor:	return "Blend_InvBlendFactor";
	case EBlend::Blend_Src1Color:		return "Blend_Src1Color";
	case EBlend::Blend_InvSrc1Color:	return "Blend_InvSrc1Color";
	case EBlend::Blend_Src1Alpha:		return "Blend_Src1Alpha";
	case EBlend::Blend_InvSrc1Alpha:	return "Blend_InvSrc1Alpha";
	default:							return "";
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
	inline RenderTargetWriteState()
		: Mask(ColorWriteFlag_All)
	{
	}

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
	Bool					BlendEnable = false;
	Bool					LogicOpEnable = false;
	EBlend					SrcBlend = EBlend::Blend_One;
	EBlend					DestBlend = EBlend::Blend_Zero;
	EBlendOp				BlendOp = EBlendOp::BlendOp_Add;
	EBlend					SrcBlendAlpha = EBlend::Blend_One;
	EBlend					DestBlendAlpha = EBlend::Blend_Zero;
	EBlendOp				BlendOpAlpha = EBlendOp::BlendOp_Add;;
	ELogicOp				LogicOp = ELogicOp::LogicOp_Noop;
	RenderTargetWriteState	RenderTargetWriteMask;
};

/*
* BlendStateCreateInfo
*/

struct BlendStateCreateInfo
{
	Bool					AlphaToCoverageEnable	= false;
	Bool					IndependentBlendEnable	= false;
	RenderTargetBlendState	RenderTarget[8];
};

/*
* BlendState
*/

class BlendState : public PipelineResource
{
public:
	BlendState()	= default;
	~BlendState()	= default;
};

/*
* InputElement
*/

enum class EInputClassification
{
	InputClassification_Vertex		= 0,
	InputClassification_Instance	= 1,
};

inline const Char* ToString(EInputClassification BlendOp)
{
	switch (BlendOp)
	{
	case EInputClassification::InputClassification_Vertex:		return "InputClassification_Vertex";
	case EInputClassification::InputClassification_Instance:	return "InputClassification_Instance";
	default:													return "";
	}
}

/*
* InputElement
*/

struct InputElement
{
	std::string				Semantic = "";
	Uint32					SemanticIndex = 0;
	EFormat					Format = EFormat::Format_Unknown;
	Uint32					InputSlot = 0;
	Uint32					ByteOffset = 0;
	EInputClassification	InputClassification = EInputClassification::InputClassification_Vertex;
	Uint32					InstanceStepRate = 0;
};

/*
* InputLayoutStateCreateInfo
*/

struct InputLayoutStateCreateInfo
{
	inline InputLayoutStateCreateInfo()
		: Elements()
	{
	}

	inline InputLayoutStateCreateInfo(const TArray<InputElement>& InElements)
		: Elements(InElements)
	{
	}

	inline InputLayoutStateCreateInfo(std::initializer_list<InputElement> InList)
		: Elements(InList)
	{
	}

	TArray<InputElement> Elements;
};

/*
* InputLayoutState
*/

class InputLayoutState : public PipelineResource
{
public:
	InputLayoutState()	= default;
	~InputLayoutState()	= default;
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
		, NumRenderTargets(0)
		, DepthStencilFormat(EFormat::Format_Unknown)
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
* GraphicsPipelineStateCreateInfo
*/

struct GraphicsPipelineStateCreateInfo
{
	InputLayoutState*			InputLayoutState		= nullptr;
	DepthStencilState*			DepthStencilState		= nullptr;
	RasterizerState*			RasterizerState			= nullptr;
	BlendState*					BlendState				= nullptr;
	Uint32						SampleCount				= 1;
	Uint32						SampleQuality			= 0;
	Uint32						SampleMask				= 0xffffffff;
	EIndexBufferStripCutValue	IBStripCutValue			= EIndexBufferStripCutValue::IndexBufferStripCutValue_Disabled;
	EPrimitiveTopologyType		PrimitiveTopologyType	= EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
	PipelineShaderState			ShaderState;
	PipelineRenderTargetFormats PipelineFormats;
};

/*
* GraphicsPipelineState
*/

class GraphicsPipelineState : public PipelineState
{
public:
	GraphicsPipelineState()		= default;
	~GraphicsPipelineState()	= default;

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
	ComputePipelineState()	= default;
	~ComputePipelineState()	= default;

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
	RayTracingPipelineState()	= default;
	~RayTracingPipelineState()	= default;

	virtual RayTracingPipelineState* AsRayTracing() override
	{
		return this;
	}

	virtual const RayTracingPipelineState* AsRayTracing() const override
	{
		return this;
	}
};