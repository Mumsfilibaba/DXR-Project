#include "MetalRHI/MetalRHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

IMPLEMENT_ENGINE_MODULE(FMetalRHIModule, MetalRHI);

FMetalRHI* FMetalRHI::GMetalRHI = nullptr;

FRHI* FMetalRHIModule::CreateRHI()
{
    TUniquePtr<FMetalRHI> NewRHI = MakeUniquePtr<FMetalRHI>();
    if (NewRHI->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewRHI.Release();
    }
}

FMetalRHI::FMetalRHI()
    : FRHI(ERHIType::Metal)
    , Device(nullptr)
    , CommandContext(nullptr)
{
    if (!GMetalRHI)
    {
        GMetalRHI = this;
    }
}

FMetalRHI::~FMetalRHI()
{
    SAFE_DELETE(CommandContext);
    SAFE_DELETE(Device);

    if (GMetalRHI == this)
    {
        GMetalRHI = nullptr;
    }
}

bool FMetalRHI::Initialize()
{
    Device = new FMetalDevice();
    if (!Device->Initialize())
    {
        METAL_ERROR("Failed to initialize FMetalDevice");
        return false;
    }

    METAL_INFO("Created FMetalDevice");

    CommandContext = new FMetalCommandContext(Device);
    if (!CommandContext->Initialize())
    {
        METAL_ERROR("Failed to initialize FMetalCommandContext");
        return false;
    }

    return true;
}

FRHITexture* FMetalRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FMetalTextureRef NewTexture = new FMetalTextureRHI(GetMetalDevice(), InTextureDesc);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FMetalRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    FMetalBufferRef NewBuffer = new FMetalBufferRHI(GetMetalDevice(), InBufferDesc);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FMetalRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
{
    FMetalSamplerStateRef NewSamplerState = new FMetalSamplerStateRHI(GetMetalDevice(), InSamplerDesc);
    if (!NewSamplerState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewSamplerState.ReleaseOwnership();
    }
}

FRHISceneAccelerationStructure* FMetalRHI::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& Desc)
{
    return new FMetalSceneAccelerationStructureRHI(GetMetalDevice(), Desc);
}

FRHIGeometryAccelerationStructure* FMetalRHI::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    return new FMetalGeometryAccelerationStructureRHI(InGeometryDesc);
}

FRHIShaderResourceView* FMetalRHI::CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc)
{
    if (InDesc.IsBufferSRV())
    {
        return new FMetalShaderResourceViewRHI(GetMetalDevice(), InDesc.BufferSRV.Buffer);
    }
    else if (InDesc.IsTextureSRV())
    {
        return new FMetalShaderResourceViewRHI(GetMetalDevice(), InDesc.TextureSRV.Texture);
    }
    else
    {
        return nullptr;
    }
}

FRHIRenderTargetView* FMetalRHI::CreateRenderTargetView(const FRHIRenderTargetViewDesc& InDesc)
{
    return new FMetalRenderTargetViewRHI(GetMetalDevice(), InDesc);
}

FRHIDepthStencilView* FMetalRHI::CreateDepthStencilView(const FRHIDepthStencilViewDesc& InDesc)
{
    return new FMetalDepthStencilViewRHI(GetMetalDevice(), InDesc);
}

FRHIUnorderedAccessView* FMetalRHI::CreateUnorderedAccessView(const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (InDesc.IsBufferUAV())
    {
        return new FMetalUnorderedAccessViewRHI(GetMetalDevice(), InDesc.BufferUAV.Buffer);
    }
    else if (InDesc.IsTextureUAV())
    {
        return new FMetalUnorderedAccessViewRHI(GetMetalDevice(), InDesc.TextureUAV.Texture);
    }
    else
    {
        return nullptr;
    }
}

FRHIComputeShader* FMetalRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    FMetalComputeShaderRef NewShader = new FMetalComputeShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIVertexShader* FMetalRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    FMetalVertexShaderRef NewShader = new FMetalVertexShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIHullShader* FMetalRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FMetalRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FMetalRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FMetalRHI::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* FMetalRHI::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* FMetalRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    FMetalPixelShaderRef NewShader = new FMetalPixelShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayGenShader* FMetalRHI::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayGenShaderRef NewShader = new FMetalRayGenShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayAnyHitShader* FMetalRHI::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayAnyHitShaderRef NewShader = new FMetalRayAnyHitShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayClosestHitShader* FMetalRHI::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayClosestHitShaderRef NewShader = new FMetalRayClosestHitShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayMissShader* FMetalRHI::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayMissShaderRef NewShader = new FMetalRayMissShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FMetalRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    FMetalDepthStencilStateRef NewDepthStencilState = new FMetalDepthStencilStateRHI(GetMetalDevice(), InDesc);
    if (!NewDepthStencilState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewDepthStencilState.ReleaseOwnership();
    }
}

FRHIRasterizerState* FMetalRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FMetalRasterizerStateRHI(InDesc);
}

FRHIBlendState* FMetalRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FMetalBlendStateRHI(InDesc);
}

FRHIInputLayout* FMetalRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FMetalInputLayoutRHI(InInputElements);
}

FRHIGraphicsPipelineState* FMetalRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    FMetalGraphicsPipelineStateRef NewPipelineState = new FMetalGraphicsPipelineStateRHI(GetMetalDevice(), InDesc);
    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }
    
    return NewPipelineState.ReleaseOwnership();
}

FRHIComputePipelineState* FMetalRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    return new FMetalComputePipelineStateRHI();
}

FRHIRayTracingPipelineState* FMetalRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Desc)
{
    return new FMetalRayTracingPipelineStateRHI();
}

FRHIQuery* FMetalRHI::CreateQuery(EQueryType InQueryType)
{
    return new FMetalQueryRHI(InQueryType);
}

FRHISwapChain* FMetalRHI::CreateSwapChain(const FRHISwapChainDesc& SwapChainDesc)
{
    FCocoaWindow* Window = reinterpret_cast<FCocoaWindow*>(SwapChainDesc.WindowHandle);
    if (!Window)
    {
        return nullptr;
    }

    FRHISwapChainDesc NewViewportDesc(SwapChainDesc);
    if (SwapChainDesc.Width == 0 || SwapChainDesc.Height == 0)
    {
        __block NSRect Frame;
        __block NSRect ContentRect;
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            Frame       = Window.frame;
            ContentRect = [Window contentRectForFrameRect:Window.frame];
        }, NSDefaultRunLoopMode, true);
        
        NewViewportDesc.Width  = ContentRect.size.width;
        NewViewportDesc.Height = ContentRect.size.height;
    }
    
    FMetalSwapChainRef NewSwapChain = new FMetalSwapChainRHI(GetMetalDevice(), NewViewportDesc);
    if (!NewSwapChain->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewSwapChain.ReleaseOwnership();
    }
}

bool FMetalRHI::QueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

bool FMetalRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    OutMemoryInfo = FRHIVideoMemoryInfo();
    return false;
}

bool FMetalRHI::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    OutResult = FRHIPipelineStatistics();
    return false;
}

void FMetalRHI::BeginFrame()
{
}

void FMetalRHI::EndFrame()
{
}

FRHIFence* FMetalRHI::CreateFence()
{
    return new FMetalFenceRHI();
}

IRHICommandContext* FMetalRHI::ObtainCommandContext()
{
    return CommandContext;
}

bool FMetalRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    OutResult = 0;
    return true;
}

void FMetalRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    // delete Resource;
}

void* FMetalRHI::GetRHINativeAdapter()
{
    // TODO: Finish
    return nullptr;
}

void* FMetalRHI::GetRHINativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetMTLDevice());
}

void* FMetalRHI::GetRHINativeDirectCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetMTLCommandQueue());
}

void* FMetalRHI::GetRHINativeComputeCommandQueue()
{
    // TODO: Finish
    return nullptr;
}

void* FMetalRHI::GetRHINativeCopyCommandQueue()
{
    // TODO: Finish
    return nullptr;
}

FString FMetalRHI::GetAdapterName() const
{
    // TODO: Finish
    return FString();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
