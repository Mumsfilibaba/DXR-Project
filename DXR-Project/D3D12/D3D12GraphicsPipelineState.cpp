#include "D3D12GraphicsPipelineState.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"

D3D12GraphicsPipelineState::D3D12GraphicsPipelineState(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, PipelineState(nullptr)
{
}

D3D12GraphicsPipelineState::~D3D12GraphicsPipelineState()
{
}

bool D3D12GraphicsPipelineState::Initialize(const GraphicsPipelineStateProperties& Properties)
{
	struct PipelineStream
	{
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
		ID3D12RootSignature*				RootSignature = nullptr;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE	Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
		D3D12_INPUT_LAYOUT_DESC				InputLayout;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE		PrimitiveTopologyType;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type3 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
		D3D12_SHADER_BYTECODE				VertexShader;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type4 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
		D3D12_SHADER_BYTECODE				PixelShader;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type5 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
		D3D12_RT_FORMAT_ARRAY				RenderTargetInfo;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type6 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
		DXGI_FORMAT							DepthBufferFormat;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type7 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
		D3D12_RASTERIZER_DESC				RasterizerDesc;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type8 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
		D3D12_DEPTH_STENCIL_DESC			DepthStencilDesc;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type9 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
		D3D12_BLEND_DESC					BlendStateDesc;

	} Pipeline = { };

	D3D12_INPUT_LAYOUT_DESC& InputLayoutDesc	= Pipeline.InputLayout;
	InputLayoutDesc.pInputElementDescs			= Properties.InputElements;
	InputLayoutDesc.NumElements					= Properties.NumInputElements;

	D3D12_SHADER_BYTECODE& VertexShader = Pipeline.VertexShader;
	VertexShader.pShaderBytecode	= reinterpret_cast<void*>(Properties.VSBlob->GetBufferPointer());
	VertexShader.BytecodeLength		= Properties.VSBlob->GetBufferSize();

	D3D12_SHADER_BYTECODE& PixelShader = Pipeline.PixelShader;
	PixelShader.pShaderBytecode	= reinterpret_cast<void*>(Properties.PSBlob->GetBufferPointer());
	PixelShader.BytecodeLength	= Properties.PSBlob->GetBufferSize();

	D3D12_RT_FORMAT_ARRAY& RenderTargetInfo = Pipeline.RenderTargetInfo;
	for (Uint32 Index = 0; Index < Properties.NumRenderTargets; Index++) 
	{
		RenderTargetInfo.RTFormats[Index] = Properties.RTFormats[Index];
	}
	RenderTargetInfo.NumRenderTargets = Properties.NumRenderTargets;

	D3D12_RASTERIZER_DESC& RasterizerDesc	= Pipeline.RasterizerDesc;
	RasterizerDesc.FillMode					= D3D12_FILL_MODE_SOLID;
	RasterizerDesc.CullMode					= D3D12_CULL_MODE_NONE;
	RasterizerDesc.FrontCounterClockwise	= FALSE;
	RasterizerDesc.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
	RasterizerDesc.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	RasterizerDesc.SlopeScaledDepthBias		= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	RasterizerDesc.DepthClipEnable			= true;
	RasterizerDesc.MultisampleEnable		= FALSE;
	RasterizerDesc.AntialiasedLineEnable	= FALSE;
	RasterizerDesc.ForcedSampleCount		= 0;
	RasterizerDesc.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc	= Pipeline.DepthStencilDesc;
	DepthStencilDesc.DepthEnable				= Properties.EnableDepth;
	DepthStencilDesc.DepthWriteMask				= D3D12_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc					= D3D12_COMPARISON_FUNC_LESS;
	DepthStencilDesc.StencilEnable				= Properties.EnableDepth;
	DepthStencilDesc.FrontFace.StencilFailOp	= DepthStencilDesc.FrontFace.StencilDepthFailOp = DepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilFunc		= D3D12_COMPARISON_FUNC_ALWAYS;
	DepthStencilDesc.BackFace					= DepthStencilDesc.FrontFace;

	D3D12_BLEND_DESC& BlendStateDesc						= Pipeline.BlendStateDesc;
	BlendStateDesc.AlphaToCoverageEnable					= false;
	BlendStateDesc.RenderTarget[0].BlendEnable				= Properties.EnableBlending;
	BlendStateDesc.RenderTarget[0].SrcBlend					= D3D12_BLEND_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].DestBlend				= D3D12_BLEND_INV_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].BlendOp					= D3D12_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].SrcBlendAlpha			= D3D12_BLEND_INV_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].DestBlendAlpha			= D3D12_BLEND_ZERO;
	BlendStateDesc.RenderTarget[0].BlendOpAlpha				= D3D12_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;

	// RootSignature
	if (Properties.RootSignature)
	{
		Pipeline.RootSignature = Properties.RootSignature->GetRootSignature();
	}

	// Topology
	Pipeline.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Depth Format
	Pipeline.DepthBufferFormat = Properties.DepthBufferFormat;

	const D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc = { sizeof(PipelineStream), &Pipeline };
	HRESULT hResult = Device->GetDXRDevice()->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
	if (SUCCEEDED(hResult))
	{
		SetName(Properties.DebugName);

		::OutputDebugString("[D3D12GraphicsPipelineState]: Created PipelineState\n");
		return true;
	}
	else 
	{
		::OutputDebugString("[D3D12GraphicsPipelineState]: FAILED to Create PipelineState\n");
		return false;
	}
}

void D3D12GraphicsPipelineState::SetName(const std::string& InName)
{
	std::wstring WideName = ConvertToWide(InName);
	PipelineState->SetName(WideName.c_str());
}
