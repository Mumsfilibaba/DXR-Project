#include "Core/Containers/UniquePtr.h"
#include "Core/Tasks/Tasks.h"
#include "D3D11RHI/D3D11RHI.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11DeviceDebug.h"
#include "D3D11RHI/D3D11Loader.h"
#include "D3D11RHI/D3D11StubResources.h"
#include "D3D11RHI/D3D11SwapChain.h"

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
    , Adapter(nullptr)
    , Device(nullptr)
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
    if (CommandContext)
    {
        CommandContext->ClearState();
        CommandContext->Flush();
    }

    SAFE_DELETE(CommandContext);

    const bool bDebugLayerEnabled = Adapter ? Adapter->IsDebugLayerEnabled() : false;
    SAFE_DELETE(Device);
    SAFE_DELETE(Adapter);

    if (bDebugLayerEnabled)
    {
        D3D11Debug::ReportLiveDXGIObjects();
    }

    D3D11::Release();

    if (D3D11DeviceRHI == this)
    {
        D3D11DeviceRHI = nullptr;
    }
}

bool FD3D11DeviceRHI::Initialize()
{
    // Load Library and Function-Pointers etc.
    if (!D3D11::Initialize())
    {
        return false;
    }

    Adapter = new FD3D11Adapter();
    if (!Adapter->Initialize())
    {
        return false;
    }

    Device = new FD3D11Device(Adapter);
    if (!Device->Initialize())
    {
        return false;
    }

    CommandContext = new FD3D11CommandContext(Device);
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
    Device->FlushDebugMessages();
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
    CHECK(InSwapChainDesc.WindowHandle != nullptr);

    if (!Tasks::IsInRHIThread())
    {
        FRHISwapChain* NewSwapChain = nullptr;
        Tasks::LaunchOnRHIThread("D3D11CreateSwapChain", [this, &NewSwapChain, &InSwapChainDesc]()
        {
            NewSwapChain = CreateSwapChain(InSwapChainDesc);
        }).Wait();

        return NewSwapChain;
    }

    FD3D11SwapChainRHIRef NewSwapChain = new FD3D11SwapChainRHI(Device, CommandContext, InSwapChainDesc);
    if (!NewSwapChain->Initialize())
    {
        return nullptr;
    }

    return NewSwapChain.ReleaseOwnership();
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
    if (!Adapter)
    {
        return false;
    }

    const DXGI_MEMORY_SEGMENT_GROUP MemoryGroup = MemoryType == EVideoMemoryType::Local ?
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL :
        DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL;

    DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemoryInfo;
    HRESULT hr = Adapter->GetDXGIAdapter3()->QueryVideoMemoryInfo(0, MemoryGroup, &VideoMemoryInfo);
    if (FAILED(hr))
    {
        D3D11_ERROR("[FD3D11DeviceRHI] QueryVideoMemoryInfo failed");
        return false;
    }

    OutMemoryInfo.MemoryType   = MemoryType;
    OutMemoryInfo.MemoryUsage  = VideoMemoryInfo.CurrentUsage;
    OutMemoryInfo.MemoryBudget = VideoMemoryInfo.Budget;
    return true;
}

bool FD3D11DeviceRHI::QueryUAVFormatSupport(EFormat Format) const
{
    ID3D11Device* D3D11Device = Device->GetD3D11Device();

    D3D11_FEATURE_DATA_D3D11_OPTIONS2 FeatureData = {};
    if (SUCCEEDED(D3D11Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &FeatureData, sizeof(FeatureData))))
    {
        if (FeatureData.TypedUAVLoadAdditionalFormats)
        {
            D3D11_FEATURE_DATA_FORMAT_SUPPORT2 FormatSupport = {};
            FormatSupport.InFormat = ConvertFormat(Format);

            const HRESULT Result = D3D11Device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &FormatSupport, sizeof(FormatSupport));
            if (FAILED(Result) || (FormatSupport.OutFormatSupport2 & D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0)
            {
                return false;
            }
        }
    }

    return true;
}

bool FD3D11DeviceRHI::QuerySupportedSampleCounts(EFormat Format, uint32& OutSampleCounts) const
{
    OutSampleCounts = 0;

    const DXGI_FORMAT DxgiFormat = ConvertFormat(Format);
    if (DxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        return false;
    }

    for (uint32 SampleCount = 1; SampleCount <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; SampleCount <<= 1)
    {
        uint32 Quality = 0;
        if (Device->QueryMultisampleQuality(DxgiFormat, SampleCount, Quality))
        {
            OutSampleCounts |= SampleCount;
        }
    }

    return OutSampleCounts != 0;
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
    CHECK(Adapter != nullptr);
    return reinterpret_cast<void*>(Adapter->GetDXGIAdapter());
}

void* FD3D11DeviceRHI::GetRHINativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D11Device());
}

String FD3D11DeviceRHI::GetAdapterName() const
{
    CHECK(Adapter != nullptr);
    return Adapter->GetDescription();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
