#include "Core/Containers/UniquePtr.h"
#include "D3D11RHI/D3D11RHI.h"
#include "D3D11RHI/D3D11Loader.h"
#include "D3D11RHI/D3D11StubResources.h"

IMPLEMENT_ENGINE_MODULE(FD3D11ModuleRHI, D3D11RHI);

DISABLE_UNREFERENCED_VARIABLE_WARNING

FD3D11DeviceRHI* FD3D11DeviceRHI::D3D11DeviceRHI = nullptr;

FRHIDevice* FD3D11ModuleRHI::CreateDevice()
{
    TUniquePtr<FD3D11DeviceRHI> NewRHI = MakeUniquePtr<FD3D11DeviceRHI>();
    if (!NewRHI->Initialize())
    {
        return nullptr;
    }

    return NewRHI.Release();
}

FD3D11DeviceRHI::FD3D11DeviceRHI()
    : FRHIDevice()
    , CommandContext(nullptr)
    , FrameNumber(0)
{
    if (!D3D11DeviceRHI)
    {
        D3D11DeviceRHI = this;
    }
}

FD3D11DeviceRHI::~FD3D11DeviceRHI()
{
    SAFE_DELETE(CommandContext);

    D3D11::Release();

    if (D3D11DeviceRHI == this)
    {
        D3D11DeviceRHI = nullptr;
    }
}

bool FD3D11DeviceRHI::Initialize()
{
    if (!D3D11::Initialize())
    {
        return false;
    }

    CommandContext = new FD3D11CommandContext(nullptr);
    if (!CommandContext->Initialize())
    {
        return false;
    }

    return InitializeDeviceFeatureSupport();
}

void FD3D11DeviceRHI::BeginFrame()
{
    CommandContext->BeginFrame();
}

void FD3D11DeviceRHI::EndFrame()
{
    CommandContext->EndFrame();
    ++FrameNumber;
}

FRHITexture* FD3D11DeviceRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData)
{
    return new FD3D11StubTextureRHI(InTextureDesc);
}

FRHIBuffer* FD3D11DeviceRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData)
{
    return new FD3D11StubBufferRHI(InBufferDesc);
}

FRHISamplerState* FD3D11DeviceRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
{
    return new FD3D11StubSamplerStateRHI(InSamplerDesc);
}

FRHISwapChain* FD3D11DeviceRHI::CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
{
    return new FD3D11StubSwapChainRHI(InSwapChainDesc);
}

FRHIQuery* FD3D11DeviceRHI::CreateQuery(EQueryType InQueryType)
{
    return new FD3D11StubQueryRHI(InQueryType);
}

FRHIFence* FD3D11DeviceRHI::CreateFence()
{
    return new FD3D11StubFenceRHI();
}

FRHIShaderResourceView* FD3D11DeviceRHI::CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
{
    return new FD3D11StubShaderResourceViewRHI(InResource, InDesc);
}

FRHIUnorderedAccessView* FD3D11DeviceRHI::CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
{
    return new FD3D11StubUnorderedAccessViewRHI(InResource, InDesc);
}

FRHIRenderTargetView* FD3D11DeviceRHI::CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
{
    return new FD3D11StubRenderTargetViewRHI(InResource, InDesc);
}

FRHIDepthStencilView* FD3D11DeviceRHI::CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
{
    return new FD3D11StubDepthStencilViewRHI(InResource, InDesc);
}

FRHIComputeShader* FD3D11DeviceRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return new TD3D11StubShaderRHI<FRHIComputeShader>();
}

FRHIVertexShader* FD3D11DeviceRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return new TD3D11StubShaderRHI<FRHIVertexShader>();
}

FRHIHullShader* FD3D11DeviceRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return new TD3D11StubShaderRHI<FRHIHullShader>();
}

FRHIDomainShader* FD3D11DeviceRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return new TD3D11StubShaderRHI<FRHIDomainShader>();
}

FRHIGeometryShader* FD3D11DeviceRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return new TD3D11StubShaderRHI<FRHIGeometryShader>();
}

FRHIPixelShader* FD3D11DeviceRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return new TD3D11StubShaderRHI<FRHIPixelShader>();
}

FRHIDepthStencilState* FD3D11DeviceRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    return new FD3D11StubDepthStencilStateRHI(InDesc);
}

FRHIRasterizerState* FD3D11DeviceRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FD3D11StubRasterizerStateRHI(InDesc);
}

FRHIBlendState* FD3D11DeviceRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FD3D11StubBlendStateRHI(InDesc);
}

FRHIInputLayout* FD3D11DeviceRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FD3D11StubInputLayoutRHI(InInputElements);
}

FRHIGraphicsPipelineState* FD3D11DeviceRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    return new TD3D11StubPipelineStateRHI<FRHIGraphicsPipelineState>();
}

FRHIComputePipelineState* FD3D11DeviceRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    return new TD3D11StubPipelineStateRHI<FRHIComputePipelineState>();
}

IRHICommandContext* FD3D11DeviceRHI::ObtainCommandContext()
{
    return CommandContext;
}

bool FD3D11DeviceRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    OutMemoryInfo              = FRHIVideoMemoryInfo();
    OutMemoryInfo.MemoryType   = MemoryType;
    return false;
}

bool FD3D11DeviceRHI::QueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

bool FD3D11DeviceRHI::QuerySupportedSampleCounts(EFormat Format, uint32& OutSampleCounts) const
{
    OutSampleCounts = RHI_SAMPLE_COUNT_1 | RHI_SAMPLE_COUNT_2 | RHI_SAMPLE_COUNT_4 | RHI_SAMPLE_COUNT_8;
    return true;
}

bool FD3D11DeviceRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    OutResult = 0;
    return true;
}

bool FD3D11DeviceRHI::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    OutResult = {};
    return true;
}

void FD3D11DeviceRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    delete Resource;
}

void* FD3D11DeviceRHI::GetRHINativeAdapter()
{
    return nullptr;
}

void* FD3D11DeviceRHI::GetRHINativeDevice()
{
    return nullptr;
}

String FD3D11DeviceRHI::GetAdapterName() const
{
    return String("D3D11 Adapter");
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
