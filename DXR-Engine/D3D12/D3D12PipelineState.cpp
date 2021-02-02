#include "D3D12PipelineState.h"

D3D12ComputePipelineState::D3D12ComputePipelineState(D3D12Device* InDevice, const TSharedRef<D3D12ComputeShader>& InShader, const TSharedRef<D3D12RootSignature>& InRootSignature)
    : ComputePipelineState()
    , D3D12DeviceChild(InDevice)
    , PipelineState(nullptr)
    , Shader(InShader)
    , RootSignature(InRootSignature)
{
}

Bool D3D12ComputePipelineState::Init()
{
    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) ComputePipelineStream
    {
        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
            ID3D12RootSignature* RootSignature = nullptr;
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
            D3D12_SHADER_BYTECODE ComputeShader = { };
        };
    } PipelineStream;

    PipelineStream.ComputeShader = Shader->GetShaderByteCode();
    PipelineStream.RootSignature = RootSignature->GetRootSignature();

    // Create PipelineState
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    Memory::Memzero(&PipelineStreamDesc, sizeof(D3D12_PIPELINE_STATE_STREAM_DESC));

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(ComputePipelineStream);

    HRESULT hResult = Device->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
    if (SUCCEEDED(hResult))
    {
        LOG_INFO("[D3D12RenderLayer]: Created ComputePipelineState");
        return true;
    }
    else
    {
        LOG_ERROR("[D3D12RenderLayer]: FAILED to Create ComputePipelineState");
        return false;
    }
}