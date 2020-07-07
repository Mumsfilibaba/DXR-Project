#include "D3D12ComputePipelineState.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"

D3D12ComputePipelineState::D3D12ComputePipelineState(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, PipelineState(nullptr)
{
}

D3D12ComputePipelineState::~D3D12ComputePipelineState()
{
}

bool D3D12ComputePipelineState::Initialize(const ComputePipelineStateProperties& Properties)
{
	struct PipelineStream
	{
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
		ID3D12RootSignature* RootSignature = nullptr;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
		D3D12_SHADER_BYTECODE ComputeShader;
	} Pipeline = { };

	// RootSignature
	if (Properties.RootSignature)
	{
		Pipeline.RootSignature = Properties.RootSignature->GetRootSignature();
	}
	
	VALIDATE(Properties.CSBlob);

	// Shader
	Pipeline.ComputeShader.BytecodeLength	= Properties.CSBlob->GetBufferSize();
	Pipeline.ComputeShader.pShaderBytecode	= Properties.CSBlob->GetBufferPointer();

	const D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc = { sizeof(PipelineStream), &Pipeline };
	HRESULT hResult = Device->GetDXRDevice()->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
	if (SUCCEEDED(hResult))
	{
		SetName(Properties.DebugName);

		::OutputDebugString("[D3D12ComputePipeline]: Created PipelineState\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12ComputePipeline]: FAILED to Create PipelineState\n");
		return false;
	}
}

void D3D12ComputePipelineState::SetName(const std::string& Name)
{
	std::wstring WideName = ConvertToWide(Name);
	PipelineState->SetName(WideName.c_str());
}
