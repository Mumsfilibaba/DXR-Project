#include "Core/Containers/UniquePtr.h"
#include "MetalRHI/MetalRHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

IMPLEMENT_ENGINE_MODULE(FMetalModuleRHI, MetalRHI);

FMetalDeviceRHI* FMetalDeviceRHI::GMetalDeviceRHI = nullptr;

FRHIDevice* FMetalModuleRHI::CreateDevice()
{
    TUniquePtr<FMetalDeviceRHI> NewRHI = MakeUniquePtr<FMetalDeviceRHI>();
    if (!NewRHI->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewRHI.Release();
    }
}

ERHIType FMetalDeviceRHI::GetRHIType() const
{
    return ERHIType::Metal;
}

FMetalDeviceRHI::FMetalDeviceRHI()
    : FRHIDevice()
    , Device(nullptr)
    , CommandContext(nullptr)
{
    if (!GMetalDeviceRHI)
    {
        GMetalDeviceRHI = this;
    }

    RHI::bSupportsDrawIndirect               = false;
    RHI::bSupportsDrawIndirectCount          = false;
    RHI::bSupportsDispatchIndirect           = false;
    RHI::bSupportsDispatchMeshIndirect       = false;
    RHI::bSupportsDispatchMeshIndirectCount  = false;
    RHI::bSupportsDispatchRaysIndirect       = false;
    RHI::MaxDrawIndirectCommandCount         = 0;
    RHI::MaxDispatchMeshIndirectCommandCount = 0;

    RHI::bSupportsSamplerFeedback            = false;
    RHI::SamplerFeedbackTier                 = ESamplerFeedbackTier::NotSupported;
}

FMetalDeviceRHI::~FMetalDeviceRHI()
{
    SAFE_DELETE(CommandContext);
    SAFE_DELETE(Device);

    if (GMetalDeviceRHI == this)
    {
        GMetalDeviceRHI = nullptr;
    }
}

bool FMetalDeviceRHI::Initialize()
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

FRHITexture* FMetalDeviceRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData)
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

FRHIBuffer* FMetalDeviceRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData)
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

FRHISamplerState* FMetalDeviceRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
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

FRHISceneAccelerationStructure* FMetalDeviceRHI::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& Desc)
{
    return new FMetalSceneAccelerationStructureRHI(GetMetalDevice(), Desc);
}

FRHIGeometryAccelerationStructure* FMetalDeviceRHI::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    return new FMetalGeometryAccelerationStructureRHI(InGeometryDesc);
}

FRHIShaderResourceView* FMetalDeviceRHI::CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
{
    if (!InResource)
    {
        return nullptr;
    }

    if (InDesc.IsBufferSRV() || InDesc.IsTextureSRV() || InDesc.IsAccelerationStructureSRV())
    {
        return new FMetalShaderResourceViewRHI(GetMetalDevice(), InResource, InDesc);
    }

    return nullptr;
}

FRHIRenderTargetView* FMetalDeviceRHI::CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
{
    if (!InResource || InResource->GetResourceType() != ERHIResourceType::Texture)
    {
        return nullptr;
    }

    FRHITexture* Texture = static_cast<FRHITexture*>(InResource);
    return new FMetalRenderTargetViewRHI(GetMetalDevice(), Texture, InDesc);
}

FRHIDepthStencilView* FMetalDeviceRHI::CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
{
    if (!InResource || InResource->GetResourceType() != ERHIResourceType::Texture)
    {
        return nullptr;
    }
    
    FRHITexture* Texture = static_cast<FRHITexture*>(InResource);
    return new FMetalDepthStencilViewRHI(GetMetalDevice(), Texture, InDesc);
}

FRHIUnorderedAccessView* FMetalDeviceRHI::CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (!InResource)
    {
        return nullptr;
    }

    if (InDesc.IsBufferUAV() || InDesc.IsTextureUAV())
    {
        return new FMetalUnorderedAccessViewRHI(GetMetalDevice(), InResource, InDesc);
    }

    return nullptr;
}

FRHIUnorderedAccessView* FMetalDeviceRHI::CreateSamplerFeedbackUnorderedAccessView(FRHITexture* /* InFeedbackTexture */, FRHITexture* /* InTargetedTexture */)
{
    METAL_ERROR("CreateSamplerFeedbackUnorderedAccessView: sampler feedback is not supported by the Metal backend");
    return nullptr;
}

FRHIComputeShader* FMetalDeviceRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
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

FRHIVertexShader* FMetalDeviceRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
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

FRHIHullShader* FMetalDeviceRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FMetalDeviceRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FMetalDeviceRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FMetalDeviceRHI::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* FMetalDeviceRHI::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* FMetalDeviceRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
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

FRHIRayGenShader* FMetalDeviceRHI::CreateRayGenShader(const TArray<uint8>& ShaderCode)
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

FRHIRayAnyHitShader* FMetalDeviceRHI::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
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

FRHIRayClosestHitShader* FMetalDeviceRHI::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
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

FRHIRayMissShader* FMetalDeviceRHI::CreateRayMissShader(const TArray<uint8>& ShaderCode)
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

FRHIRayIntersectionShader* FMetalDeviceRHI::CreateRayIntersectionShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayIntersectionShaderRef NewShader = new FMetalRayIntersectionShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayCallableShader* FMetalDeviceRHI::CreateRayCallableShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayCallableShaderRef NewShader = new FMetalRayCallableShaderRHI(GetMetalDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FMetalDeviceRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
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

FRHIRasterizerState* FMetalDeviceRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FMetalRasterizerStateRHI(InDesc);
}

FRHIBlendState* FMetalDeviceRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FMetalBlendStateRHI(InDesc);
}

FRHIInputLayout* FMetalDeviceRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FMetalInputLayoutRHI(InInputElements);
}

FRHIGraphicsPipelineState* FMetalDeviceRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    FMetalGraphicsPipelineStateRef NewPipelineState = new FMetalGraphicsPipelineStateRHI(GetMetalDevice(), InDesc);
    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }
    
    return NewPipelineState.ReleaseOwnership();
}

FRHIComputePipelineState* FMetalDeviceRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    return new FMetalComputePipelineStateRHI();
}

FRHIMeshletPipelineState* FMetalDeviceRHI::CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc)
{
    // Mesh shaders (object/mesh) are not yet implemented on Metal.
    return nullptr;
}

FRHIRayTracingPipelineState* FMetalDeviceRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Desc)
{
    return new FMetalRayTracingPipelineStateRHI();
}

FRHIQuery* FMetalDeviceRHI::CreateQuery(EQueryType InQueryType)
{
    return new FMetalQueryRHI(InQueryType);
}

FRHISwapChain* FMetalDeviceRHI::CreateSwapChain(const FRHISwapChainDesc& SwapChainDesc)
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

bool FMetalDeviceRHI::QueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

bool FMetalDeviceRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    OutMemoryInfo = FRHIVideoMemoryInfo();
    return false;
}

bool FMetalDeviceRHI::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    OutResult = FRHIPipelineStatistics();
    return false;
}

void FMetalDeviceRHI::BeginFrame()
{
}

void FMetalDeviceRHI::EndFrame()
{
}

FRHIFence* FMetalDeviceRHI::CreateFence()
{
    return new FMetalFenceRHI();
}

IRHICommandContext* FMetalDeviceRHI::ObtainCommandContext()
{
    return CommandContext;
}

bool FMetalDeviceRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    OutResult = 0;
    return true;
}

void FMetalDeviceRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    // delete Resource;
}

void* FMetalDeviceRHI::GetRHINativeAdapter()
{
    // TODO: Finish
    return nullptr;
}

void* FMetalDeviceRHI::GetRHINativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetMTLDevice());
}

void* FMetalDeviceRHI::GetRHINativeDirectCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetMTLCommandQueue());
}

void* FMetalDeviceRHI::GetRHINativeComputeCommandQueue()
{
    // TODO: Finish
    return nullptr;
}

void* FMetalDeviceRHI::GetRHINativeCopyCommandQueue()
{
    // TODO: Finish
    return nullptr;
}

String FMetalDeviceRHI::GetAdapterName() const
{
    // TODO: Finish
    return String();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
